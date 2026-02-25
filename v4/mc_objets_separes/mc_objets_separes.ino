/**
*  @brief : Programme de traitement d'un Module Connecté ESP32
* 
*            Mise en oeuvre de classes en fichiers separes
* 
*
*   
*  @author :cgil - @2026
* @version : v4.1 - Modulable & Configurable
 * @details : Utilisation de ConfigManager pour supprimer les constantes en dur
            Reset usine si bouton BOOT pressé au démarrage
*  
*  @details: Programmation en Objet
        - serveur web : separation du code HTML, css dans /DATA // arborescence Web standard
           . linux : utilisez le script 'upload_web.sh' après chaque MAJ
           . windows : 'Ctrl+sh+p' lancez "Upload LittleFS" via l'IDE arduino 2.x
               ! vérifiez dossier "arduino_littlefs_upload" dans ...user/.arduinoIDE/plugins
               (ESP8266LittleFS-2.6.0.zip par exemple)

        - serveur BD : wrapper Dao pourencapsuler les requetes SQL
           . Migration de SQLite vers Preferences (equivalent à la base de registre windows)
           . 120 cles/valeurs max pour 1 enreg / 30 s

        - Mise en place d'un buffer circulaire de 120 mesures (1 heure de données).

        - Synchronisation automatique de l'heure du navigateur vers l'ESP32.

        - V4.1 : Pour Debug Wifi en mode Hybride

*
*
* @details mode d'emploi serveur Web
CONFIGURATION DU MODE RESEAU :
         - Module en réseau étoile   AVEC SCMC
         - Module en solo            SANS SCMC

 * CONFIGURATION PREMIER LANCEMENT : 
    wifi : code SSID : "SSID_GMC_PWD_2026" , Pwd : "2026"
    allez sur : http://192.168.4.1/config

 * En mode Solo :
     WiFi : Votre PC doit maintenant se connecter au réseau WiFi .
     Accès : Dans votre navigateur,
  
   1 : Connectez votre PC/Smartphone au WiFi "SSID_GMC_MC01" et password "PWD_MC01"
   2 : Ouvrez votre navigateur
        tapez http://192.168.4.1 (l'IP par défaut d'un ESP32 en mode Point d'Accès).
        allez sur : http://192.168.4.1/api/status  JSON


         Double usage : * Si vous allez sur /,
     vous voyez un joli petit tableau (le HTML est généré par l'ESP,
         code très court donc rapide à téléverser).


       Si vous allez sur /api/status vous avez toujours votre JSON.
         Donc, si plus tard vous décidez d'allumer un SCMC,
         ce dernier pourra toujours interroger le module sans rien changer !


           resultat en JSON
           [
           {"id_mesure":133,"date_creation":"2026-02-13 12:34:52","valeur_tdc":2150},
           {"id_mesure":132,"date_creation":"2026-02-13 12:34:22","valeur_tdc":2480},
           {"id_mesure":131,"date_creation":"2026-02-13 12:33:51","valeur_tdc":1870}
           ]
*/


/**
*  \brief : classe NetworkManagerGMC inline
* 
*  @ classe de gestion du réseau SOLO ou en CLUSTER
*   --- IS_SOLO
*      SOLO    : "SSID_GMC_MC01" et password "PWD_MC01" sur 192.168.4.1
*      CLUSTER : "SSID_GMC_SCMC" et password "PWD_SCMC" sur 192.168.4.xx (10, 11, 12...)
*/


#include <WiFi.h>
#include <WebServer.h>
#include "WebManager.h"
#include "daoGmc.h"
#include "mesure.h"
#include "setup_on.h"
#include "ConfigManager.h" 


// Objets globaux via pointeurs
ConfigManager* configManager=nullptr; 
WebServer server(80);
DaoGMC* dao = nullptr;
WebManager* webManager = nullptr;
SetupOn* so = nullptr;


/**
 * @brief : declaration des fonctions internes
 */
void gestionMesures(); //! Simule ou enregistre une mesure
void setLED(uint8_t r, uint8_t g, uint8_t b); //! simplifier les couleurs
void checkFactoryReset(); //! Gère le Reset Usine



void setup() {
    // --- ÉTAPE 1 : BOOT & CONFIG ---
    setLED(128, 40, 0); // Orange
    Serial.begin(115200);
    configManager = new ConfigManager();
    configManager->begin();
    
    // Reset usine (Bouton BOOT)
    pinMode(0, INPUT_PULLUP);
    if (digitalRead(0) == LOW) configManager->factoryReset();

    // --- ÉTAPE 2 : HORLOGE & DAO ---
    so = new SetupOn();
    so->synchroniserHorlogeAuSetup();
    dao = new DaoGMC("/littlefs/gmc.db");
    dao->begin();

    // --- ÉTAPE 3 : RÉSEAU & WEB (Le coeur du système) ---
    setLED(128, 0, 128); // Violet
    webManager = new WebManager(server, configManager);
    
    if (webManager->begin()) {
        webManager->setupNetwork(); 
        webManager->setupRoutes();
        server.begin();
        Serial.println("Système Réseau & Web : OK");
    }

    // --- FIN : PRÊT ---
    setLED(0, 128, 0); // Vert
}


void setupv4() {
    // --- ÉTAPE 1.1 : ORANGE (BOOT)  & CONFIG ---
    setLED(128, 40, 0); // Orange
    delay(2000); // Pour voir la couleur

    //! --- ÉTAPE 1.2 : preparer le moniteur serie
    Serial.begin(115200);
    delay(1000);

    unsigned long start = millis();
    while (!Serial && (millis() - start < 3000));
    Serial.println("\n--- DEMARRAGE SYSTEME GMC V4---");
    setLED(192, 40, 0);
    delay(2000); 

    //! --- ÉTAPE 1.2.1 : Initialisation de la config
    // Initialisation de la configuration (Lecture Flash NVS)
    configManager = new ConfigManager();
    configManager->begin();
    Serial.printf("ID UNIQUE ESP32 : %s\n", configManager->getSSID().c_str());
    
    // Reset usine si bouton BOOT pressé au démarrage
    pinMode(0, INPUT_PULLUP);
    if (digitalRead(0) == LOW) {
        setLED(255, 0, 0); // Rouge Flash
        configManager->factoryReset(); 
    }

    //! --- ÉTAPE 1.3 : synchro horloge
    Serial.println("Synchronisation horloge ... "); 
    so = new SetupOn(); 
    if (so->synchroniserHorlogeAuSetup())
        Serial.println(">>> Horloge ESP32 calée sur l'heure du PC !");
    else
        Serial.println(">>> ! Erreur de synchro Horloge ESP32");
    setLED(64, 40, 0);
    delay(2000); 


   // --- ÉTAPE 2 : BLEU (DATABASE / DAO) ---
    setLED(0, 0, 128); // Bleu
   Serial.println("Initialisation DAO (Preferences)...");
   dao = new DaoGMC("/littlefs/gmc.db");
   if (dao->begin()) {
       Serial.println("DAO Init: OK");
   }
   delay(2000); // Pour voir la couleur


   // --- ÉTAPE 4 : VIOLET : RÉSEAU DYNAMIQUE ---
    setLED(128, 0, 128); // Violet
   // On récupère le mode et les identifiants depuis l'objet config
    if (configManager->getMode() == "solo") { 
        Serial.printf("Mode SOLO - AP: %s\n", configManager->getSSID().c_str());
        WiFi.softAPdisconnect(true); // On coupe les anciennes connexions
        
        #ifdef DEBUG_HOME_BOX   //! ==> Mode Hybride Wifi pour tests internes
            // Configurer le mode HYBRIDE (Access Point + Station)
            WiFi.mode(WIFI_AP_STA);
        #else
            WiFi.mode(WIFI_AP);          // ON FORCE LE MODE ACCESS POINT
        #endif
        delay(100);                  // Petit temps de pause pour la puce WiFi
        
        //! Sécurité : si le mot de passe est trop court, on passe en réseau ouvert
        String pass = configManager->getPassword();
        const char* passStr = pass.c_str();
        // Sécurité : si le mot de passe est trop court, on passe en réseau ouvert
        if (pass.length() < 8) {
            Serial.println("ATTENTION : Mot de passe trop court (<8). Passage en mode OUVERT.");
            passStr = NULL; 
        }

        bool success = WiFi.softAP(configManager->getSSID().c_str(), passStr);
        if(success) {
            Serial.print("Point d'accès créé avec succès. IP : ");
            Serial.println(WiFi.softAPIP());
        } else {
            Serial.println("Échec de création du Point d'accès !");
        }

        // Connexion à votre Box (STA)
        #ifdef DEBUG_HOME_BOX   //! ==> Mode Hybride Wifi pour tests internes
        WiFi.begin(DEGUB_HOME_BOX_SSID, DEGUB_HOME_BOX_PWD);
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 20) {
            delay(500);
            Serial.print(".");
            retry++;
        }
        if(WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnecté à la Box !");
            Serial.print("IP via la Box : "); Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nÉchec connexion Box (mais le mode AP reste actif)");
        }
        #endif

    } else {
        Serial.printf("Mode CLUSTER - Connexion à: %s\n", configManager->getSSID().c_str());
        WiFi.begin(configManager->getSSID().c_str(), configManager->getPassword().c_str());
        
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 20) {
            delay(500); Serial.print("."); retry++;
        }
    }
    Serial.println("\nReseau OK.");
   delay(500); // Pour voir la couleur


    //! --- ÉTAPE 5 : SERVEUR WEB --- 
   webManager = new WebManager(server, configManager);
   // AJOUTE CETTE LIGNE ICI :
   if (webManager->begin()) {
        webManager->setupRoutes();
         server.begin();
       Serial.println("WebManager : LittleFS chargé avec succès !");
   } else {
       Serial.println("WebManager : Erreur critique LittleFS !");
       exit(-2);
   }
  

   // --- ÉTAPE fin : FLASH VERT (PRÊT) ---
   setLED(0, 128, 0);
   delay(1000);
  
}

void loop() {
    // 1. Gérer les requêtes Web (Toujours en priorité)
    server.handleClient();

    // 2. Surveillance du bouton Reset (Non-bloquant)
    checkFactoryReset();

    // 3. Gestion de l'échantillonnage des mesures
    gestionMesures();
}



/*** FONCTIONS INTERNES ****/

/**
 * Gère le Reset Usine si le bouton BOOT est maintenu 5s
 */
void checkFactoryReset() {
    static unsigned long buttonPressTime = 0;
    static bool isPressing = false;

    if (digitalRead(0) == LOW) { // Bouton enfoncé
        if (!isPressing) {
            buttonPressTime = millis();
            isPressing = true;
            Serial.println("Bouton BOOT détecté...");
        }

        unsigned long duration = millis() - buttonPressTime;
        
        if (duration > 5000) {
            setLED(255, 0, 0); // Rouge Fixe
            configManager->factoryReset(); 
        } else if (duration > 1000) {
            // Feedback visuel : clignotement
            if ((millis() / 100) % 2) setLED(255, 0, 0); 
            else setLED(0, 0, 0);
        }
    } else if (isPressing) {
        // Relâché avant les 5s
        isPressing = false;
        // Restaurer couleur de veille
        if (configManager->getMode() == "solo") setLED(30, 5, 0);
        else setLED(0, 0, 30);
    }
}

/**
 * Simule ou enregistre une mesure selon la fréquence définie en config
 */
void gestionMesures() {
    static unsigned long lastUpdate = 0;
    
    // On récupère dynamiquement la fréquence depuis la config
    unsigned long intervalle = configManager->getFreq() * 1000;

    if (millis() - lastUpdate >= intervalle) { 
        lastUpdate = millis();
        
        setLED(0, 255, 0); // Flash vie
        
        // Simulation de mesure
        int val = random(180, 260);
        
        if(dao->accederTableMesure_ecrireUneMesure(val)) {
           Serial.printf("Mesure stockée : %d\n", val);
        }
        
        delay(100); // Petit flash
        if (configManager->getMode() == "solo") setLED(30, 5, 0);
        else setLED(0, 0, 30);
    }
}

/**
* Fonction utilitaire pour simplifier les couleurs
*/
void setLED(uint8_t r, uint8_t g, uint8_t b) {
   neopixelWrite(RGB_BUILTIN, r, g, b);
}

