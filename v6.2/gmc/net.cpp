/**
 * @brief   code de la classe modele pour 
				gerer le web et la page web
 * @file    net.cpp
 * @author  cgil 
 * @version 1.1 
 * @date    fev 2026

  - serveur web : separation du code HTML, css dans /DATA // arborescence Web standard
           . linux : utilisez le script 'upload_web.sh' après chaque MAJ
           . windows : 'Ctrl+sh+p' lancez "Upload LittleFS" via l'IDE arduino 2.x
               ! vérifiez dossier "arduino_littlefs_upload" dans ...user/.arduinoIDE/plugins
               (ESP8266LittleFS-2.6.0.zip par exemple)
 */

 

#include <WiFi.h>  
#include <ArduinoJson.h>
#include <HTTPClient.h>


#include "net.h"
#include "dbg.h"

// On n'oublie pas de dire que dao existe ailleurs
extern Dao* dao; 

Net::Net(WebServer& webServer, Conf* config) 
    : _webServer(webServer), _conf(config) {}


/**
    @brief : méthode pour envoyer la donnée
*/
void Net::sendToCloud(float valTemp, bool etatVoyant, String cloudUrl) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

         Serial.print("☁️ Tentative d'envoi vers le cloud ["); Serial.print(cloudUrl); Serial.println("]...");
        
        // URL de ton nouveau dossier sur Alwaysdata
        //http.begin("http://btscielinfo.alwaysdata.net/projet/index.php");
        http.begin(cloudUrl);
        http.addHeader("Content-Type", "application/json");

        // On prépare le JSON
        // ex: {"temp": 215, "voyant": true}
        String json = "{";
        json += "\"temp\":" + String(valTemp) + ",";
        json += "\"voyant\":" + String(etatVoyant ? "true" : "false");
        json += "}";

        // Envoi du POST
        int httpResponseCode = http.POST(json);

        if (httpResponseCode > 0) {
            Serial.print("✅ Cloud OK, code : ");
            Serial.println(httpResponseCode);
        } else {
            Serial.print("❌ Erreur envoi Cloud : ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
}


void Net::gererEnvoiDataCloud() {
    static unsigned long dernierEnvoi = 0;

    // On utilise la valeur de frequences
    unsigned long intervalle = this->_conf->getFrequenceMesures()*1000;
    
    if (millis() - dernierEnvoi >= intervalle) { 
        dernierEnvoi = millis();

        int derniereValTemp = 0;
        std::vector<Mesure> mesures = dao->accederTableMesure_lireDesMesures(1);
        if (!mesures.empty()) 
            derniereValTemp = mesures[0].getValeurTdc();
        //bool monEtat = digitalRead(PIN_ALERTE); // Exemple de booléen
        bool monEtatVoyant = false;

        // On utilise l'URL stockée dans les Prefs !
        sendToCloud(derniereValTemp, monEtatVoyant, _conf->getBoxCloudUrl());
    }
}


bool Net::begin() {
   Serial.print("\t🛠 Montage LittleFS :");
    
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
    Serial.print("\t\t.Fichiers :");
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
        Serial.println("\nAction spécifique sur GPIO demandée par le Web");
        //digitalWrite(40, HIGH); // Rouge
        // Exemple d'action : digitalWrite(4, HIGH);
        _webServer.send(200, "text/plain", "GPIO Actionne avec succes");
    });

    _webServer.on("/api/piloter_gpio/semaphore_on", HTTP_GET, [this](){
        Serial.println("\nAction spécifique sur GPIO demandée par le Web");
        Serial.println("🚨 Allumage du Sémaphore !");
    
        // On allume les 3 LEDs (adapte les numéros de pins selon ton câblage)
        digitalWrite(40, HIGH); // Rouge
        digitalWrite(41, HIGH); // Jaune
        digitalWrite(42, HIGH); // Vert
       

        
        // Exemple d'action : digitalWrite(4, HIGH);
        _webServer.send(200, "text/plain", "GPIO Actionne avec succes  Sémaphore Allumé");
    });

    _webServer.on("/api/piloter_gpio/semaphore_off", HTTP_GET, [this](){
        Serial.println("\nAction spécifique sur GPIO demandée par le Web");
        Serial.println("🚨 Eteint le Sémaphore !");
    
        // On eteint les 3 LEDs (adapte les numéros de pins selon ton câblage)
        digitalWrite(40, LOW); // Rouge
        digitalWrite(41, LOW); // Jaune
        digitalWrite(42, LOW); // Vert
        
        // Exemple d'action : digitalWrite(4, HIGH);
        _webServer.send(200, "text/plain", "GPIO Actionne avec succes  Sémaphore eteint");
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
    

    // [ROUTE API CONFIG] : Envoie les réglages actuels au formulaire HTML
    _webServer.on("/api/config", HTTP_GET, [this]() {
        JsonDocument doc;
        
        // On remplit le JSON avec les valeurs de ton objet de config
        doc["box_ssid"] = _conf->getBoxSSID();
        doc["box_pwd"]  = _conf->getBoxPassword();
        doc["box_cloud_url"]  = _conf->getBoxCloudUrl();
        doc["ap_ssid"]  = _conf->getApSSID();
        doc["ap_pwd"]   = _conf->getApPassword();
        doc["freq"]     = _conf->getFrequenceMesures();
        doc["mode"]     = _conf->getMode();

        String response;
        serializeJson(doc, response);
        _webServer.send(200, "application/json", response);
        
        Serial.println("📡 API : Données de config envoyées au navigateur");
    });

    // [ROUTE API SAVE] : Reçoit les réglages du formulaire et les sauvegarde
    _webServer.on("/api/config", HTTP_POST, [this]() {
        Serial.println("📥 Réception d'une nouvelle configuration...");

        // On récupère les valeurs envoyées par le formulaire JS
        String new_box_ssid = _webServer.arg("box_ssid");
        String new_box_pwd  = _webServer.arg("box_pwd");
        String new_box_cloud_url  = _webServer.arg("box_cloud_url");
        String new_ap_ssid  = _webServer.arg("ap_ssid");
        String new_ap_pwd   = _webServer.arg("ap_pwd");
        int new_freq        = _webServer.arg("freq").toInt();
        String new_mode   = _webServer.arg("mode");

        // On met à jour l'objet de configuration (qui va écrire dans les Preferences)
        // Note : Adapte le nom de ta méthode de sauvegarde si elle est différente
        _conf->save(new_box_ssid, new_box_pwd, new_box_cloud_url, 
                        new_ap_ssid, new_ap_pwd, 
                        new_freq, new_mode);

        // On répond au navigateur pour confirmer que c'est OK
        JsonDocument doc;
        doc["status"] = "success";
        doc["message"] = "Configuration enregistrée. Redémarrage...";
        
        String response;
        serializeJson(doc, response);
        _webServer.send(200, "application/json", response);

        // On laisse un peu de temps pour que la réponse arrive au navigateur
        // avant de couper le WiFi pour redémarrer.
        Serial.println("💾 Sauvegarde effectuée. Reboot dans 2 secondes.");
        delay(2000);
        ESP.restart();
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
    Serial.println("\t📅 Réseau Dynamique");
    
    if (_conf->getMode() == "solo") { 
        //Serial.printf("\t\t-Mode SOLO - AP: %s\n", _conf->getSSID().c_str());
        

        /** 
            1. Nettoyage complet pour repartir sur une base saine
        */
        WiFi.softAPdisconnect(true); 
        WiFi.disconnect(true);
        delay(500); // On laisse un peu plus de temps à la puce

        /** 
            2. On essaie d' etre toujours en mode hybride
                sinon on laisse tomber et on garde que le mode AP
        */ 
        Serial.printf("\t\t.Tentative mode STA Hybride - AP et Box ...\n");
        WiFi.mode(WIFI_AP_STA); 

        Serial.printf("\t\t.Connexion box : [%s] ...\n", this->_conf->getBoxSSID().c_str());
        Serial.print("\t\t.Adresse MAC Station : "); Serial.println(WiFi.macAddress());
        Serial.print("\t\t.Adresse MAC SoftAP  : "); Serial.println(WiFi.softAPmacAddress());
        Serial.printf("\t\t.Mode actuel : %d (1:STA, 2:AP, 3:BOTH)\n", WiFi.getMode());

        // 2.1 : On force la désactivation du mode économie (aide à l'auth)
        WiFi.setSleep(false); 
        WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);
        WiFi.begin(this->_conf->getBoxSSID().c_str(), this->_conf->getBoxPassword().c_str());
    
        //! 2.2 : Tentative connexion a la Box locale pendant 10 secondes
        int retry = 0;  Serial.printf("\t\t\t");
        while (WiFi.status() != WL_CONNECTED && retry < 20) { 
            delay(500); Serial.print("... "); retry++; 
        }

        //! 2.3 : Connexte Box ? Oui ou Non
        if(WiFi.status() == WL_CONNECTED) {
                Serial.println(" ->Connecté à la Box ✅");
                Serial.printf("\t🏠 Box : %s\n", WiFi.localIP().toString().c_str());
                Serial.print("\t\t.Cloud URL : ["); Serial.print(this->_conf->getBoxCloudUrl()); Serial.println("]");
        }

        //! Box impossible ou pas configuree
        //! Pour configurer : 192.168.4.1/config
        if(WiFi.status() != WL_CONNECTED) {
            Serial.println("\n\t\t Pas de Box possible !. Mode Hybride abandonné");
            Serial.println("\t\t⚠️ Changer la config avec '192.168.4.1/config'");
            
            
            //! MODE NORMAL : Uniquement Point d'accès
                //! 1. Nettoyage complet pour repartir sur une base saine
            
            WiFi.softAPdisconnect(true); 
            WiFi.disconnect(true);
            delay(500); // On laisse un peu plus de temps à la puce

            WiFi.mode(WIFI_AP); //Uniquement Point d'accès
        }
        
      
        //! chp'tite pause...
        delay(200);

        // --- CONFIGURATION DU POINT D'ACCÈS (AP) ---
        // On le fait après la connexion Box (si box trouvee) pour hériter du bon canal WiFi
        String password_AP = _conf->getApPassword();
        const char* passord_AP_Str = (password_AP.length() < 8) ? NULL : password_AP.c_str();

        if (WiFi.softAP(_conf->getApSSID().c_str(), passord_AP_Str)) {
            Serial.print("\t📡 Point d'accès IP : "); Serial.print(WiFi.softAPIP()); 
            Serial.print(" - SSID ["); Serial.print(_conf->getApSSID()); Serial.print("] "); Serial.println("✅");
        } else {
            Serial.println("\t❌ ERREUR : Échec création mode AP.");
            haltSystem(); 
        }

    } else {
        // --- MODE CLUSTER ---
        Serial.printf("\t\tMode CLUSTER - Connexion à: %s\n", _conf->getApSSID().c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(_conf->getApSSID().c_str(), _conf->getApPassword().c_str());
        
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



