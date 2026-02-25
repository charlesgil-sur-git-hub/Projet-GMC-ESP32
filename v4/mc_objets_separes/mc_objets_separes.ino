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
#include "debugGmc.h"

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
void panic(String message);


void setup() {
    // --- ÉTAPE 1 : BOOT & CONFIG ---
    setLED(128, 40, 0); // Orange
    Serial.begin(115200);
    configManager = new ConfigManager();
    configManager->begin();
    
    // Reset usine (Bouton BOOT)
    pinMode(0, INPUT_PULLUP);
    if (digitalRead(0) == LOW) 
        configManager->factoryReset();

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
    } else {
        // SI LE FILESYSTEM NE MONTE PAS, ON ARRÊTE TOUT ICI !
        panic("Echec initialisation LittleFS / WebManager");
    }

    // --- FIN : PRÊT ---
    setLED(0, 128, 0); // Vert
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

/**
@brief fonction panic() qui allume la LED en rouge et bloque tout
*/
// --- FONCTION DE SÉCURITÉ GLOBALE ---
void panic(String message) {
    setLED(255, 0, 0); // Passage immédiat au ROUGE
    Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.print("!!! ERREUR CRITIQUE : ");
    Serial.println(message);
    Serial.println("!!! SYSTÈME HALTÉ (BOUCLE INFINIE)");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    
    while(true) {
        // Optionnel : faire clignoter la LED rouge pour attirer l'attention
        setLED(255, 0, 0); delay(500);
        setLED(0, 0, 0);   delay(500);
        Serial.println("❌ mode panic ...");
    }
}
 
