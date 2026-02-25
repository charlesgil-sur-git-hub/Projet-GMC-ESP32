#include "WebManager.h"
#include "debugGmc.h"
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

/***                                                            *****/
/***                                                            *****/

//******    PARTIE           WIFI                               **////

/***                                                            *****/
/***                                                            *****/

 /**
*  @brief : Parametres Wifi voir "ConfigManager.h"
* 
*  @ classe de gestion du réseau SOLO ou en CLUSTER
*   --- IS_SOLO
*      SOLO    : "SSID_GMC_MC01" et password "PWD_MC01" sur 192.168.4.1
*      CLUSTER : "SSID_GMC_SCMC" et password "PWD_SCMC" sur 192.168.4.xx (10, 11, 12...)

    EN DEBUG : voir le fichier source "debugGmc.h"
    ex tests en meme temps sur http://192.168.1.48/ (home freebox)
*/


void WebManager::setupNetwork() {
    Serial.println("\n--- Configuration Réseau Dynamique ---");
    
    if (_configManager->getMode() == "solo") { 
        Serial.printf("Mode SOLO - AP: %s\n", _configManager->getSSID().c_str());
        
        // 1. Nettoyage complet pour repartir sur une base saine
        WiFi.softAPdisconnect(true); 
        WiFi.disconnect(true);
        delay(500); // On laisse un peu plus de temps à la puce

        #ifdef DEBUG_GMC_HOME_BOX 
            // MODE HYBRIDE : On cherche la BOX d'abord
            WiFi.mode(WIFI_AP_STA); 
            Serial.println(">>> DEBUG MODE ACTIVE : Tentative de connexion Box PRIORITAIRE");
            
            /*  //! les détails du scan de tous les réseaux 
            Serial.println("Scan complet des réseaux en cours...");
            int n = WiFi.scanNetworks();
            Serial.printf("%d réseaux trouvés :\n", n);
            if (n == 0) {
                Serial.println(">>> AUCUN RÉSEAU TROUVÉ. Problème d'antenne ou de puissance ?");
            } else {
                for (int i = 0; i < n; ++i) {
                    Serial.printf("%d: %s (Ch:%d, RSSI:%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
                }
            }
            */

            Serial.printf("Connexion box : %s ...", DEBUG_GMC_HOME_BOX_SSID);

            Serial.println("--- Diagnostics WiFi ---");
            Serial.print("Adresse MAC Station : "); Serial.println(WiFi.macAddress());
            Serial.print("Adresse MAC SoftAP  : "); Serial.println(WiFi.softAPmacAddress());
            Serial.printf("Mode actuel : %d (1:STA, 2:AP, 3:BOTH)\n", WiFi.getMode());

            // On force la désactivation du mode économie (aide à l'auth)
            WiFi.setSleep(false); 
            WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);
            WiFi.begin(DEBUG_GMC_HOME_BOX_SSID, DEBUG_GMC_HOME_BOX_PWD);
            
            int retry = 0;
            while (WiFi.status() != WL_CONNECTED && retry < 30) { 
                delay(500); 
                Serial.print("."); 
                retry++; 
            }

            if(WiFi.status() == WL_CONNECTED) {
                Serial.println("\n[OK] Connecté à la Box !");
                //Serial.print("IP Station : "); Serial.println(WiFi.localIP());
                Serial.println("   RÉSEAU ÉTABLI - RÉSUMÉ DES ACCÈS");
                Serial.println("========================================");
                Serial.printf(" 🏠 MODE BOX (Station)    : http://%s\n", WiFi.localIP().toString().c_str());
                Serial.printf(" 📡 MODE DIRECT (AP)      : http://%s\n", WiFi.softAPIP().toString().c_str());
                Serial.println("========================================\n");

            } else {
                Serial.println("\n[ERREUR] Box introuvable ou mauvais mot de passe.");
                haltSystem(); // On bloque en rouge
            }
        #else
            // MODE NORMAL : Uniquement Point d'accès
            WiFi.mode(WIFI_AP); 
        #endif
        
        delay(200);

        // --- 2. CONFIGURATION DU POINT D'ACCÈS (AP) ---
        // On le fait après la connexion Box pour hériter du bon canal WiFi
        String pass = _configManager->getPassword();
        const char* passStr = (pass.length() < 8) ? NULL : pass.c_str();

        if (WiFi.softAP(_configManager->getSSID().c_str(), passStr)) {
            Serial.print("Point d'accès OK. IP : "); Serial.println(WiFi.softAPIP());
        } else {
            Serial.println("ERREUR : Échec création AP.");
            haltSystem(); 
        }

    } else {
        // --- MODE CLUSTER ---
        Serial.printf("Mode CLUSTER - Connexion à: %s\n", _configManager->getSSID().c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(_configManager->getSSID().c_str(), _configManager->getPassword().c_str());
        
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 30) { 
            delay(500); Serial.print("."); retry++; 
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\n[ERREUR] Impossible de joindre le Cluster.");
            haltSystem();
        }
        Serial.println("\n[OK] Connecté au Cluster !");
    }
}



// Fonction utilitaire pour bloquer le système en cas d'erreur

void WebManager::haltSystem() {
    Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("!!! ❌ SYSTÈME HALTÉ : ERREUR RÉSEAU  !!!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    
    while (true) {
        neopixelWrite(RGB_BUILTIN, 255, 0, 0); // ROUGE
        delay(500);
        neopixelWrite(RGB_BUILTIN, 0, 0, 0); // ÉTEINT
        delay(500);
        
        // On réinitialise le watchdog si nécessaire pour éviter un reboot automatique
        yield(); 
    }
}



