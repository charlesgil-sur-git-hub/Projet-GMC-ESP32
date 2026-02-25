#include "WebManager.h"
#include <WiFi.h>  
#include <ArduinoJson.h>

// On n'oublie pas de dire que dao existe ailleurs
extern DaoGMC* dao; 

WebManager::WebManager(WebServer& server, ConfigManager* config) 
    : _webServer(server), _configManager(config) {}

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
    _webServer.on("/config", HTTP_GET, [this]() {
        File file = LittleFS.open("/config.html", "r");
        if (file) {
            _webServer.streamFile(file, "text/html");
            file.close();
        } else {
            _webServer.send(404, "text/plain", "Fichier config.html manquant");
        }
    });

    // --- 2. L'URL pour la MACHINE (API) ---
    // En GET : On envoie les données
    _webServer.on("/api/config", HTTP_GET, [this]() {
        String json = "{";
        json += "\"ssid\":\"" + _configManager->getSSID() + "\",";
        json += "\"freq\":" + String(_configManager->getFreq()) + ",";
        json += "\"mode\":\"" + _configManager->getMode() + "\"";
        json += "}";
        _webServer.send(200, "application/json", json);
    });

    // En POST : On reçoit et on enregistre
    _webServer.on("/api/config", HTTP_POST, [this]() {
        // Sauvegarde via les arguments du formulaire reçu
        _configManager->save(
            _webServer.arg("ssid"),
            _webServer.arg("pass"),
            _webServer.arg("freq").toInt(),
            _webServer.arg("mode")
        );
        
        // On répond au JavaScript avant de couper la connexion
        _webServer.send(200, "application/json", "{\"status\":\"success\"}");
        
        Serial.println("Config mise à jour. Redémarrage...");
        delay(2000);
        ESP.restart();
    });
    
    // Route API JSON : On va chercher la      VRAIE dernière mesure !
    _webServer.on("/api/status", HTTP_GET, [this]() {
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
        _webServer.send(200, "application/json", response);
    });

    // Gestion des fichiers statiques (index.html, etc.)
    _webServer.onNotFound([this]() {
        if (!handleFileRead(_webServer.uri())) {
            _webServer.send(404, "text/plain", "Fichier introuvable sur LittleFS");
        }
    });

    //!synchroniserHorlogeAuSetup
    _webServer.on("/api/sync_time", HTTP_GET, [this]() {
        if (_webServer.hasArg("t")) {
            time_t t = _webServer.arg("t").toInt();
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, NULL); // L'ESP32 est maintenant pile à l'heure du PC !
            _webServer.send(200, "text/plain", "OK");
        }
    });

    _webServer.on("/api/history", HTTP_GET, [this]() {
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
        _webServer.send(200, "application/json", response);
    });
    
    _webServer.on("/api/led", HTTP_GET, [this]() {
        // 1. Lire l'état actuel et l'inverser
        int etatActuel = digitalRead(2); // On lit le GPIO 2
        int nouvelEtat = !etatActuel;
        digitalWrite(2, nouvelEtat);

        // 2. Répondre au navigateur avec le nouvel état
        JsonDocument doc;
        doc["status"] = (nouvelEtat == HIGH) ? "ON" : "OFF";
        
        String response;
        serializeJson(doc, response);
        _webServer.send(200, "application/json", response);
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
        _webServer.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

//******   WIFI    **////
/**
    @brief Mode Debug home Box Oui/Non
*/
#define DEBUG_HOME_BOX   
//! identifiants de Box (pour le mode Station)
#define DEBUG_HOME_BOX_SSID     "Freebox_34F871"
#define DEBUG_HOME_BOX_PWD      "touballoles"

void WebManager::setupNetwork() {
    Serial.println("--- Configuration Réseau Dynamique ---");
    
    if (_configManager->getMode() == "solo") { 
        Serial.printf("Mode SOLO - AP: %s\n", _configManager->getSSID().c_str());
        WiFi.softAPdisconnect(true); 
        
        #ifdef DEBUG_HOME_BOX 
            WiFi.mode(WIFI_AP_STA); // Mode Hybride
        #else
            WiFi.mode(WIFI_AP); 
        #endif
        
        delay(100);
        
        String pass = _configManager->getPassword();
        const char* passStr = (pass.length() < 8) ? NULL : pass.c_str();

        if (WiFi.softAP(_configManager->getSSID().c_str(), passStr)) {
            Serial.print("Point d'accès OK. IP : "); Serial.println(WiFi.softAPIP());
        }

        #ifdef DEBUG_HOME_BOX 
            WiFi.begin(DEBUG_HOME_BOX_SSID, DEBUG_HOME_BOX_PWD);
            int retry = 0;
            while (WiFi.status() != WL_CONNECTED && retry < 20) { delay(500); Serial.print("."); retry++; }
            if(WiFi.status() == WL_CONNECTED) {
                Serial.print("\nConnecté Box! IP: "); Serial.println(WiFi.localIP());
            }
        #endif

    } else {
        Serial.printf("Mode CLUSTER - Connexion à: %s\n", _configManager->getSSID().c_str());
        WiFi.begin(_configManager->getSSID().c_str(), _configManager->getPassword().c_str());
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 20) { delay(500); Serial.print("."); retry++; }
    }
}
