#include "WebManager.h"
#include <ArduinoJson.h>

// On n'oublie pas de dire que dao existe ailleurs
extern DaoGMC* dao; 

WebManager::WebManager(WebServer& server) : _server(server) {}

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
    // Route API JSON : On va chercher la VRAIE dernière mesure !
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
