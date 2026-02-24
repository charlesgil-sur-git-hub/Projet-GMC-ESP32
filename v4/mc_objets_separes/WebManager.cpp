#include "WebManager.h"
#include <ArduinoJson.h>

// On n'oublie pas de dire que dao existe ailleurs
extern DaoGMC* dao; 

WebManager::WebManager(WebServer& server, ConfigManager* config) 
    : _server(server), _config(config) {}

bool WebManager::begin() {
   Serial.println("Tentative de montage de LittleFS...");
    
    // Le 'true' ici est vital : il force le formatage si l'image est mal lue
    if (!LittleFS.begin(true)) {
        Serial.println("❌ Erreur : Impossible de monter LittleFS");
        return false;        
    }
    Serial.print("Total LittleFS: ");
    Serial.println(LittleFS.totalBytes());
    Serial.print("Used LittleFS: ");
    Serial.println(LittleFS.usedBytes());

    Serial.println("✅ LittleFS monté avec succès !");
    
    // Scan des fichiers
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    if(!file) Serial.println("   (Le système de fichiers est vide)");
    while(file){
        Serial.printf("   Fichier: %s, Taille: %u octets\n", file.name(), file.size());
        file = root.openNextFile();
    }
    return true;
}

void WebManager::setupRoutes() {
    // --- 1. L'URL pour les HUMAINS ---
    _server.on("/config", HTTP_GET, [this]() {
        File file = LittleFS.open("/config.html", "r");
        if (file) {
            _server.streamFile(file, "text/html");
            file.close();
        } else {
            _server.send(404, "text/plain", "Fichier config.html manquant");
        }
    });

    // --- 2. L'URL pour la MACHINE (API) ---
    // En GET : On envoie les données
    _server.on("/api/config", HTTP_GET, [this]() {
        String json = "{";
        json += "\"ssid\":\"" + _config->getSSID() + "\",";
        json += "\"freq\":" + String(_config->getFreq()) + ",";
        json += "\"mode\":\"" + _config->getMode() + "\"";
        json += "}";
        _server.send(200, "application/json", json);
    });

    // En POST : On reçoit et on enregistre
    _server.on("/api/config", HTTP_POST, [this]() {
        // Sauvegarde via les arguments du formulaire reçu
        _config->save(
            _server.arg("ssid"),
            _server.arg("pass"),
            _server.arg("freq").toInt(),
            _server.arg("mode")
        );
        
        // On répond au JavaScript avant de couper la connexion
        _server.send(200, "application/json", "{\"status\":\"success\"}");
        
        Serial.println("Config mise à jour. Redémarrage...");
        delay(2000);
        ESP.restart();
    });
    
    // Route API JSON : On va chercher la      VRAIE dernière mesure !
    _server.on("/api/status", HTTP_GET, [this]() {
        JsonDocument doc; 
        
        // --- CONNEXION À LA DAO LIGHT ---
        // On récupère la dernière mesure (la liste en contient une seule dans notre simulation)
        std::vector<Mesure> mesures = dao->accederTableMesure_lireDesMesures(1);
        
        if (!mesures.empty()) {
            doc["temp"] = mesures[0].getValeurTdc(); // Conversion tdc vers Celsius
            doc["date"] = mesures[0].getDateCreation();
            doc["id"]   = mesures[0].getIdMesure();
        } else {
            doc["temp"] = 0;
            doc["msg"] = "Aucune donnée en base";
        }

        doc["led_status"] = "ACTIVE";
        doc["uptime"] = millis() / 1000;
        doc["mode"] = "PREFERENCES_MODE"; // Pour savoir qu'on ne tourne pas sur SQLite

        String response;
        serializeJson(doc, response);
        _server.send(200, "application/json", response);
    });

    // Gestion des fichiers statiques (index.html, etc.)
    _server.onNotFound([this]() {
        if (!handleFileRead(_server.uri())) {
            _server.send(404, "text/plain", "Fichier introuvable sur LittleFS");
        }
    });

    //!synchroniserHorlogeAuSetup
    _server.on("/api/sync_time", HTTP_GET, [this]() {
        if (_server.hasArg("t")) {
            time_t t = _server.arg("t").toInt();
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, NULL); // L'ESP32 est maintenant pile à l'heure du PC !
            _server.send(200, "text/plain", "OK");
        }
    });

    _server.on("/api/history", HTTP_GET, [this]() {
        // Utilisation d'un document assez large pour 120 mesures
        JsonDocument doc; 
        JsonArray history = doc.to<JsonArray>();
        
        // On demande les 120 dernières mesures au DAO
        std::vector<Mesure> mesures = dao->accederTableMesure_lireDesMesures(120);
        
        for (auto& m : mesures) {
            JsonObject obj = history.add<JsonObject>();
            obj["v"] = m.getValeurTdc() / 10.0; // La valeur
            obj["t"] = m.getDateCreation();    // L'heure (HH:MM:SS)
        }

        String response;
        serializeJson(doc, response);
        _server.send(200, "application/json", response);
    });
    
    _server.on("/api/led", HTTP_GET, [this]() {
        // 1. Lire l'état actuel et l'inverser
        int etatActuel = digitalRead(2); // On lit le GPIO 2
        int nouvelEtat = !etatActuel;
        digitalWrite(2, nouvelEtat);

        // 2. Répondre au navigateur avec le nouvel état
        JsonDocument doc;
        doc["status"] = (nouvelEtat == HIGH) ? "ON" : "OFF";
        
        String response;
        serializeJson(doc, response);
        _server.send(200, "application/json", response);
    });
}

String WebManager::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css"))  return "text/css";
    if (filename.endsWith(".js"))   return "application/javascript";
    if (filename.endsWith(".ico"))  return "image/x-icon";
    return "text/plain";
}

bool WebManager::handleFileRead(String path) {
    if (path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);

    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        _server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}
