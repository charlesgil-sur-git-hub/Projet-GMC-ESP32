/**
 * @brief   code de la classe modele pour 
				gerer le web et la page web
 * @file    net.cpp
 * @author  cgil 
 * @version 1.1 
 * @date    fev 2026
 */

#include <WiFi.h>  
#include <ArduinoJson.h>

#include "net.h"
#include "dbg.h"

// On n'oublie pas de dire que dao existe ailleurs
extern Dao* dao; 

Net::Net(WebServer& webServer, Conf* config) 
    : _webServer(webServer), _conf(config) {}

bool Net::begin() {
   Serial.print("\t->> Montage LittleFS :");
    
    // Le 'true' ici est vital : il force le formatage si l'image est mal lue
    if (!LittleFS.begin(true)) {
        Serial.println("❌ Erreur FS");
        return false;        
    }
    Serial.print("Total ["); Serial.print(LittleFS.totalBytes()); Serial.print("]");
    Serial.print(" - Used ["); Serial.print(LittleFS.usedBytes());  Serial.print("]");
    Serial.println(" OK ✅");
    
    // Scan des fichiers
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    if(!file) {
        Serial.println("\t ❌ système de fichiers vide");
        return false; 
    }
    Serial.print("\t\t->>> Fichiers :");
    while(file){
        Serial.printf("{%s}[%u] ", file.name(), file.size());
        file = root.openNextFile();
    }
    Serial.println("✅");
    return true;
}

/*void Net::setupRoutes() {
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
        json += "\"ssid\":\"" + _conf->getSSID() + "\",";
        json += "\"freq\":" + String(_conf->getFreq()) + ",";
        json += "\"mode\":\"" + _conf->getMode() + "\"";
        json += "}";
        _webServer.send(200, "application/json", json);
    });

    // En POST : On reçoit et on enregistre
    _webServer.on("/api/config", HTTP_POST, [this]() {
        // Sauvegarde via les arguments du formulaire reçu
        _conf->save(
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
    
     // --- ROUTE POUR L'ACTION LED (Bouton 1) ---
    _webServer.on("/api/flash", HTTP_GET, [this]() {
         Serial.println("🚨 Action LED demandée !");
        
        // Ton code pour faire flasher la LED orange
        // flashOrange(); 

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

   

    // --- ROUTE POUR LA TEMPÉRATURE ---
    _webServer.on("/api/get_temp", HTTP_GET, [this](){
        Serial.println("🌐 Requête reçue : /api/get_temp");

        // 1. On récupère la vraie valeur (via ton DAO par exemple)
        float t = 22.5; // Ici tu mettras : _dao->getLastTemp();
        String h = "10:45"; // Ici : _base->getFormattedTime();

        // 2. On prépare la réponse au format JSON (le langage du JS)
        // On crée une chaîne : {"temp": 22.5, "heure": "10:45"}
        String json = "{";
        json += "\"temp\":" + String(t) + ",";
        json += "\"heure\":\"" + h + "\"";
        json += "}";

        // 3. On envoie la réponse au navigateur
        _webServer.send(200, "application/json", json);
    });

    _webServer.on("/api/piloter_gpio", HTTP_GET, [this](AsyncWebServerRequest *request){
        Serial.println("🚨 Bouton  piloter_gpio !");

        // Votre code C++ ici (ex: piloter le GPIO )
        _webServer.send(200, "text/plain", "Action reçue !");
    });
}
*/

/**
 * @section GUIDE_PEDAGOGIQUE_V5
 * * COMMENT AJOUTER UNE NOUVELLE INTERACTION (EX: UN BOUTON) :
 * * 1. DANS LE HTML : Créer l'élément visuel
 * <button onclick="maFonction()">Mon Action</button>
 * * 2. DANS LE JS (script.js) : Créer la fonction d'appel
 * function maFonction() { 
 * fetch('/api/ma-route').then(res => res.json()).then(data => ...); 
 * }
 * * 3. DANS LE C++ (ici) : Créer le "Slot" (la Route)
 * _webServer.on("/api/ma-route", HTTP_GET, [this](){
 * // Faire l'action C++ (ex: digitalWrite)
 * _webServer.send(200, "application/json", "{\"status\":\"ok\"}");
 * });
 */
void Net::setupRoutes() {

    // --- 1. ROUTES POUR LES PAGES (Interface Utilisateur) ---
    
    _webServer.on("/", HTTP_GET, [this]() {
        if (!handleFileRead("/index.html")) {
            _webServer.send(404, "text/plain", "index.html introuvable");
        }
    });

    _webServer.on("/config", HTTP_GET, [this]() {
        handleFileRead("/config.html");
    });


    // --- 2. API : ROUTES DE DONNÉES (Le "Back-end") ---

    // [ROUTE STATUS] : Appelée automatiquement toutes les 15s par le timer JS
    _webServer.on("/api/status", HTTP_GET, [this]() {
        JsonDocument doc; 
        
        // Récupération de la dernière mesure via le DAO
        std::vector<Mesure> mesures = dao->accederTableMesure_lireDesMesures(1);
        
        if (!mesures.empty()) {
            doc["temp"] = mesures[0].getValeurTdc(); // Valeur brute (ex: 215 pour 21.5)
            doc["date"] = mesures[0].getDateCreation();
        } else {
            doc["temp"] = 0;
            doc["date"] = "--:--";
        }

        doc["uptime"] = millis() / 1000;
        
        String response;
        serializeJson(doc, response);
        _webServer.send(200, "application/json", response);
    });

   

    // [ROUTE GET_UPTIME] :  Reçoit une valeur et répond
    _webServer.on("/api/get_uptime", HTTP_GET, [this]() {
        String message = "Aucune valeur";
         Serial.print("/api/get_uptime...");
        
        // On vérifie si l'argument "valeur" est présent dans l'URL
        if (_webServer.hasArg("valeur")) {
            message = _webServer.arg("valeur");
            Serial.print("📥 Valeur uptime reçue du Web : ");
            Serial.println(message);
            
            // Exemple : on fait clignoter la LED selon la valeur reçue
            // flashLED(message.toInt()); 
        }

        JsonDocument doc;
        doc["status"] = "OK";
        doc["recu"] = message; // On renvoie la valeur pour confirmation
        
        String response;
        serializeJson(doc, response);
        _webServer.send(200, "application/json", response);
    });

    // [ROUTE PILOTER] : Action générique sur GPIO
    _webServer.on("/api/piloter_gpio", HTTP_GET, [this](){
        Serial.println("Action spécifique sur GPIO demandée par le Web");
        // Exemple d'action : digitalWrite(4, HIGH);
        _webServer.send(200, "text/plain", "GPIO Actionne avec succes");
    });

    // [ROUTE SYNC] : Reçoit l'heure du navigateur au chargement
    _webServer.on("/api/sync_time", HTTP_GET, [this]() {
        if (_webServer.hasArg("t")) {
            time_t t = _webServer.arg("t").toInt();
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, NULL); 
            _webServer.send(200, "text/plain", "Heure synchronisee");
        }
    });


    // --- 3. GESTION DES FICHIERS STATIQUES & ERREURS ---

    _webServer.onNotFound([this]() {
        if (!handleFileRead(_webServer.uri())) {
            _webServer.send(404, "text/plain", "404: Fichier non trouve");
        }
    });
}


String Net::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css"))  return "text/css";
    if (filename.endsWith(".js"))   return "application/javascript";
    if (filename.endsWith(".ico"))  return "image/x-icon";
    return "text/plain";
}

bool Net::handleFileRead(String path) {
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

/***                                          *****/
/***                                          *****/

//******    PARTIE           WIFI             **////

/***                                          *****/
/***                                      ...    *****/

 /**
*  @brief : Parametres Wifi voir "conf.h"
* 
*  @ classe de gestion du réseau SOLO ou en CLUSTER
*   --- IS_SOLO
*      SOLO    : "SSID_GMC_MC01" et password "PWD_MC01" sur 192.168.4.1
*      CLUSTER : "SSID_GMC_SCMC" et password "PWD_SCMC" sur 192.168.4.xx (10, 11, 12...)

 * CONFIGURATION PREMIER LANCEMENT : voir le SSID du wifi et le Pwd est la fin du SSID
    wifi : code SSID : "SSID_GMC_PASS_1234XXXX" , Pwd : "1234XXXX"
    allez sur : http://192.168.4.1/config

    EN DEBUG : voir le fichier initurce "dbg.h"
    ex tests en meme temps sur http://192.168.1.48/ (home freebox)
*/


void Net::setupNetwork() {
    Serial.println("\t->> Réseau Dynamique");
    
    if (_conf->getMode() == "solo") { 
        Serial.printf("\t\t-Mode SOLO - AP: %s\n", _conf->getSSID().c_str());
        
        // 1. Nettoyage complet pour repartir sur une base saine
        WiFi.softAPdisconnect(true); 
        WiFi.disconnect(true);
        delay(500); // On laisse un peu plus de temps à la puce

        //! en mode hybride ?
        this->boxSsid="";
        this->boxPwd="";
        #ifdef DEBUG_GMC_BOX_HYBRIDE
            // MODE HYBRIDE : On cherche la BOX d'abord
            WiFi.mode(WIFI_AP_STA); 
            Serial.println("\t\t\t> DEBUG MODE ACTIVE : Tentative de connexion Box PRIORITAIRE");

            #ifdef DEBUG_GMC_S9_PARTAGE 
                this->boxSsid = DEBUG_GMC_S9_PARTAGE_SSID;
                Serial.printf("\t\t\t\t📥Saisir mot de passe du ssid[%s] ? ", this->boxSsid);
                // Attendre que le port série soit prêt (utile pour certaines cartes comme Leonardo/Micro)
                while (!Serial);
                while (Serial.available() == 0);
                // Une fois qu'il y a des données, on les lit
                this->boxPwd = Serial.readStringUntil('\n');
                this->boxPwd.trim(); // Nettoie les espaces/retours à la ligne
            #endif    
            #ifdef DEBUG_GMC_LABOINFO_BOX 
                this->boxSsid = DEBUG_GMC_LABOINFO_BOX_SSID;
                this->boxPwd = DEBUG_GMC_LABOINFO_BOX_PWD;
            #endif
            #ifdef DEBUG_GMC_HOME_BOX 
                    this->boxSsid = DEBUG_GMC_HOME_BOX_SSID;
                      this->boxSsid = DEBUG_GMC_LABOINFO_BOX_SSID;
                    Serial.printf("\t\t\t📥Saisir mot de passe du ssid[%s] ? ", this->boxSsid);
                    // Attendre que le port série soit prêt (utile pour certaines cartes comme Leonardo/Micro)
                    while (!Serial);
                    while (Serial.available() == 0);
                    // Une fois qu'il y a des données, on les lit
                    this->boxPwd = Serial.readStringUntil('\n');
                    this->boxPwd.trim(); // Nettoie les espaces/retours à la ligne
            #endif
            Serial.println("");
           
            
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

            Serial.printf("\t\t\t\t.Connexion box : [%s] ...\n", this->boxSsid);
            Serial.print("\t\t\t\t.Adresse MAC Station : "); Serial.println(WiFi.macAddress());
            Serial.print("\t\t\t\t.Adresse MAC SoftAP  : "); Serial.println(WiFi.softAPmacAddress());
            Serial.printf("\t\t\t\t.Mode actuel : %d (1:STA, 2:AP, 3:BOTH)\n", WiFi.getMode());

            // On force la désactivation du mode économie (aide à l'auth)
            WiFi.setSleep(false); 
            WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);
            WiFi.begin(this->boxSsid, this->boxPwd);
            
            int retry = 0;
            while (WiFi.status() != WL_CONNECTED && retry < 30) { 
                delay(500); 
                Serial.print("."); 
                retry++; 
            }

            if(WiFi.status() == WL_CONNECTED) {
                Serial.println("\t\t->>Connecté à la Box ✅");
                //Serial.print("IP Station : "); Serial.println(WiFi.localIP());
                Serial.println("\t\t\t. RÉSEAU ÉTABLI - RÉSUMÉ DES ACCÈS");
                Serial.println("\t\t\t.========================================");
                Serial.printf("\t\t\t. 🏠 MODE BOX (Station)    : http://%s\n", WiFi.localIP().toString().c_str());
                Serial.printf("\t\t\t. 📡 MODE DIRECT (AP)      : http://%s\n", WiFi.softAPIP().toString().c_str());
                Serial.println("\t\t\t.========================================");

            } else {
                Serial.println("\n❌ [ERREUR] Box introuvable ou mauvais mot de passe.");
                haltSystem(); // On bloque en rouge
            }
        #else
            // MODE NORMAL : Uniquement Point d'accès
            WiFi.mode(WIFI_AP); 
        #endif
        
        delay(200);

        // --- 2. CONFIGURATION DU POINT D'ACCÈS (AP) ---
        // On le fait après la connexion Box pour hériter du bon canal WiFi
        String pass = _conf->getPassword();
        const char* passStr = (pass.length() < 8) ? NULL : pass.c_str();

        if (WiFi.softAP(_conf->getSSID().c_str(), passStr)) {
            Serial.print("\tPoint d'accès OK. IP : "); Serial.print(WiFi.softAPIP()); Serial.println("✅");
        } else {
            Serial.println("\t❌ ERREUR : Échec création AP.");
            haltSystem(); 
        }

    } else {
        // --- MODE CLUSTER ---
        Serial.printf("\t\tMode CLUSTER - Connexion à: %s\n", _conf->getSSID().c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(_conf->getSSID().c_str(), _conf->getPassword().c_str());
        
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 30) { 
            delay(500); Serial.print("."); retry++; 
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\t\t❌ [ERREUR] Impossible de joindre le Cluster.");
            haltSystem();
        }
        Serial.println("\t\t[OK] Connecté au Cluster ✅");
    }
}



// Fonction utilitaire pour bloquer le système en cas d'erreur

void Net::haltSystem() {
    Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("!!! ❌ SYSTÈME HALTÉ : ERREUR RÉSEAU  !!!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    
    while (true) {
        neopixelWrite(RGB_BUILTIN, 255, 0, 0); // ROUGE
        delay(500);
        neopixelWrite(RGB_BUILTIN, 0, 0, 0); // ÉTEINT
        delay(500);
        
        // On réinitialise le watchdog si nécessaire pour éviter un reinit automatique
        yield(); 
    }
}



