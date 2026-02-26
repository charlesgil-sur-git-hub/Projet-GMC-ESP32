/**
*  @brief : Programme de traitement d'un Module Connecté ESP32
* 
*            Mise en oeuvre de classes en fichiers separes
* 
*
*  @author :cgil - @2026
*  @version : v5.0 - page web "interactive" 
*  @details : gérer la communication bidirectionnelle entre le navigateur (JavaScript) et ton ESP32 (C++).
*  @details : Utilisation de Conf pour supprimer les constantes en dur
            Reset usine si bouton BOOT pressé au démarrage
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

        - Release Pour Debug Wifi en mode Hybride

*
* 
*
* @details mode d'emploi serveur Web
CONFIGURATION DU MODE RESEAU :
         - Module en réseau étoile   AVEC SCMC
         - Module en initlo            SANS SCMC

 * CONFIGURATION PREMIER LANCEMENT : voir le SSID du wifi et le Pwd est la fin du SSID
    wifi : code SSID : "SSID_GMC_PASS_1234XXXX" , Pwd : "1234XXXX"
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
		   
*
* @details Developpement : architecture
	gmc.ino (Cœur)
	conf.h/cpp (Réglages JSON)
	dao.h/cpp (Gestion des mesures)
	net.h/cpp (Serveur Web & WiFi)
	dbg.h/cpp (Mode hybride et outils de test)
*
*/

#include <WiFi.h>
#include <WebServer.h>


#include "conf.h" 
#include "net.h"
#include "dao.h"
#include "mesure.h"
#include "dbg.h"

//! Objets globaux via pointeurs
WebServer webServer(80);

Conf* conf=nullptr; 
Dao* dao = nullptr;
Net* net = nullptr;

/**
 * @brief : declaration des fonctions internes
 */
bool syncDateTime();        //! Synchro Date Time
void detectResetConf();     //! Reset parametres conf
void simulMesures();        //! Simule une mesure par freq
void setLED(int, int, int); //! Simplifier les couleurs
void stopSetup(String);     //! Stopper setup si erreurs


void setup() {
    //! --- ÉTAPE 1 led orange : CONF ---
    setLED(128, 40, 0); // Orange
    Serial.begin(115200);

    //! conf : parametres 
    conf = new Conf();
    conf->begin();
    //! conf : reset (Bouton BOOT 5s)
    pinMode(0, INPUT_PULLUP);
    /*if (digitalRead(0) == LOW) 
        conf->factoryReset();
    */

    //! --- ÉTAPE 2 led jaune : SYNC DATE ---
    setLED(255, 255, 0); // jaune
    if (syncDateTime())
        Serial.println("Sync Horloge ESP32 ... OK");

    //! --- ÉTAPE 3 led gris : DAO ---
    setLED(206, 206, 206); // gris
    dao = new Dao("/littlefs/gmc.db");
    dao->begin();

    // --- ÉTAPE 4 led violet : RÉSEAU & WEB (Le coeur du système) ---
    setLED(128, 0, 128); // Violet
    net = new Net(webServer, conf);
    
    if (net->begin()) {
        net->setupNetwork(); 
        net->setupRoutes();
        webServer.begin();
        Serial.println("Système Réseau & Web ... OK");
    } else {
        // SI LE FILESYSTEM NE MONTE PAS, ON ARRÊTE TOUT ICI !
        stopSetup("Echec initialisation LittleFS / net");
    }

    // --- FIN : PRÊT ---
    setLED(0, 128, 0); // Vert
}



void loop() {
    // 1. Gérer les requêtes Web (Toujours en priorité)
    webServer.handleClient();

    // 2. Surveillance du bouton "boot" 5s pour reset conf (Non-bloquant)
    detectResetConf();

    // 3. Gestion de l'échantillonnage des mesures
    simulMesures();
}



/*** FONCTIONS INTERNES ****/

/**
 * Gère le Reset des params conf si le bouton BOOT est maintenu 5s
 */
void detectResetConf() {
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
            conf->factoryReset(); 
        } else if (duration > 1000) {
            // Feedback visuel : clignotement
            if ((millis() / 100) % 2) setLED(255, 0, 0); 
            else setLED(0, 0, 0);
        }
    } else if (isPressing) {
        // Relâché avant les 5s
        isPressing = false;
        // Restaurer couleur de veille
        if (conf->getMode() == "initlo") setLED(30, 5, 0);
        else setLED(0, 0, 30);
    }
}

/**
 * @brief : Simule ou enregistre une mesure selon la fréquence définie en config
 */
void simulMesures() {
    static unsigned long lastUpdate = 0;
    
    // On récupère dynamiquement la fréquence depuis la config
    unsigned long intervalle = conf->getFreq() * 1000;

    if (millis() - lastUpdate >= intervalle) { 
        lastUpdate = millis();
        
        setLED(0, 255, 0); // Flash vie
        
        // Simulation de mesure
        int val = random(180, 260);
        
        if(dao->accederTableMesure_ecrireUneMesure(val)) {
           Serial.printf("Mesure stockée : %d\n", val);
        }
        
        delay(100); // Petit flash
        if (conf->getMode() == "initlo") setLED(30, 5, 0);
        else setLED(0, 0, 30);
    }
}

/**
* Fonction util pour simplifier les couleurs
*/
void setLED(int r, int g, int b) {
   neopixelWrite(RGB_BUILTIN, (uint8_t)r, (uint8_t)g, (uint8_t)b);
}

/**
@brief fonction de sécurité dans le setup 
    allume la LED en rouge et bloque tout
*/
void stopSetup(String message) {
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

/**
 * @brief synchronise la date et l heure de l horloge PC<=>Esp32 
 */
bool syncDateTime() {
    struct tm tm_compile;
	Serial.println("Synchronisation horloge ... ");  
    // On récupère les macros de compilation __DATE__ et __TIME__
    // Format : "Feb  6 2026" et "12:34:56"
    if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm_compile)) {
        time_t t = mktime(&tm_compile);
        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        //Serial.println(">>> Horloge ESP32 calée sur l'heure du PC !");
        return true;
    }

    return false;
}
