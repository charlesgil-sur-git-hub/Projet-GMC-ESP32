/**
*  @brief : Programme de traitement d'un Module Connecté ESP32
* 
*            Mise en oeuvre de classes en fichiers separes
* 
*
*  @author :cgil - @2026
*  @version : v6.2 - Version simplifiee sans DAO
*  @details : gérer la communication bidirectionnelle entre le navigateur (JavaScript) et ton ESP32 (C++).
*  @details : Utilisation de Conf pour supprimer les constantes en dur
            Reset usine si bouton BOOT pressé au démarrage
*  @details: Programmation en Objet
        - serveur web : separation du code HTML, css dans /DATA // arborescence Web standard
           . linux : utilisez le script 'upload_web.sh' après chaque MAJ
           . windows : 'Ctrl+sh+p' lancez "Upload LittleFS" via l'IDE arduino 2.x
               ! vérifiez dossier "arduino_littlefs_upload" dans ...user/.arduinoIDE/plugins
               (ESP8266LittleFS-2.6.0.zip par exemple)

        - mini stockage de données avec Pref (nvs)
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
         - Module en solo            SANS SCMC

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
	net.h/cpp (Serveur Web & WiFi)
*
*/

#include <WiFi.h>
#include <WebServer.h>


#include "conf.h" 
#include "net.h"

//! Objets globaux via pointeurs
WebServer webServer(80);

Conf* conf=nullptr; 
Net* net = nullptr;



/**
 * @brief : declaration des fonctions internes
 */
void setLED(int, int, int); //! Simplication couleur LED
bool syncDateTime();        //! Synchro Date Time
void detectResetConf();     //! Reset parametres conf
void simulMesures();        //! Simule une mesure par freq
void stopSetup(String);     //! Stopper setup si erreurs


void setup() {
    //! --- ÉTAPE 1 led oransge : CONF ---
    setLED("orange"); delay(1000); 
    Serial.begin(115200);
    Serial.println("\n\n🚀 Demarrage Programme GMC-ESP32");

    //! conf : parametres
    Serial.print("conf ..."); 
    conf = new Conf();
    if (conf->begin())
        Serial.println("✅");
    else
        stopSetup("Erreur : conf");
    //! conf : reset (Bouton BOOT 5s)
    pinMode(0, INPUT_PULLUP);
    delay(1000);

    //! --- ÉTAPE 2 led jaune : SYNC DATE ---
    Serial.print("syncDateTime ..."); 
    setLED("jaune"); delay(2000);
    if (syncDateTime())
        Serial.println("✅");
    else
        stopSetup("Erreur : Sync Horloge ESP32");
    

    //! --- ÉTAPE 3 led gris : NO-DAO ---
    Serial.println("⚠️ NO Dao ...✅");
    setLED("gris"); delay(1000);
    

    // --- ÉTAPE 4 led violet : RÉSEAU & WEB (Le coeur du système) ---
    Serial.println("Système Réseau & Web - NET : 🔍");
    setLED("violet"); delay(2000);
    net = new Net(webServer, conf);
    if (net->begin()) {
        net->setupNetwork(); 
        net->setupRoutes();
        webServer.begin();
        Serial.println("Système Réseau & Web - NET ✅");
    } else {
        // SI LE FILESYSTEM NE MONTE PAS, ON ARRÊTE TOUT ICI !
        stopSetup("Erreur : Système Réseau & Web");
    }

    //! Pilotage GPIO : test avec 3 leds semaphore
    pinMode(40, OUTPUT); pinMode(41, OUTPUT); pinMode(42, OUTPUT);

    // --- Infos Reset Config ---
     Serial.println("\n⚠️ Appui long 5s bouton boot en clignotant rouge pour reset config ⚠️\n");

    // --- FIN : PRÊT ---
    Serial.println("");
    setLED("vert"); 
}



void loop() {
    // 1. Gérer les requêtes Web (Toujours en priorité)
    webServer.handleClient();

    // 2. Surveillance du bouton "boot" 5s pour reset conf (Non-bloquant)
    detectResetConf();

    // 3. Gestion de l'échantillonnage des mesures
    simulMesures();

    // 4. Gestion envoi data dans le cloud
    net->gererEnvoiDataCloud();
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
            Serial.println("🚨 Bouton BOOT détecté...");
        }

        unsigned long duration = millis() - buttonPressTime;
        
        if (duration > 5000) {
            setLED("rouge");
            conf->factoryReset(); 
        } else if (duration > 1000) {
            // Feedback visuel : clignotement
            if ((millis() / 100) % 2) setLED("rouge"); 
            else setLED("blanc");
        }
    } else if (isPressing) {
        // Relâché avant les 5s
        isPressing = false;
        // Restaurer couleur de veille
        setLED("vert"); // Vert
    }
}

/**
 * @brief : Simule ou enregistre une mesure selon la fréquence définie en config
 */
void simulMesures() {
    static unsigned long lastUpdate = 0;
    
    // On récupère dynamiquement la fréquence depuis la config
    unsigned long intervalle = conf->getFrequenceMesures() * 1000;

    if (millis() - lastUpdate >= intervalle) { 
        lastUpdate = millis();
        
        // petit Flash vie
        setLED("blanc");delay(100);setLED("orange");delay(200);setLED("vert");
        
        // Simulation de mesure 
        int val = random(180, 260);
        //! Simulation alerte voyant FALSE
        bool alerteVoyant=false;    
        //! ecriture NO DAO
        //if(dao->accederTableMesure_ecrireUneMesure(val)) {
        // On sauvegarde directement dans les Preferences via l'objet Conf
        conf->saveDerniereMesure(val, alerteVoyant);
        Serial.printf("🌡️ Mesure simulee : %d", val); Serial.println("\tAlerte voyant Faux");
        Serial.println("💾 Mesure sauvegardée dans les Prefs (NVS)");
    }
}

/**
* Fonction util pour simplifier les couleurs
*/
void setLED(int r, int g, int b) {
   neopixelWrite(RGB_BUILTIN, (uint8_t)r, (uint8_t)g, (uint8_t)b);
}
/** @brief : Simplication couleur LED
    vert(0,128,0) vert bouteille( 9, 106, 9) 
*/
void setLED(String color) {
    if (!color.compareTo("")) neopixelWrite(RGB_BUILTIN, 255, 0, 0); // erreur rouge
    else if (!color.compareTo("rouge")) neopixelWrite(RGB_BUILTIN, 255, 0, 0); 
    else if (!color.compareTo("orange")) neopixelWrite(RGB_BUILTIN, 30, 5, 0); 
    else if (!color.compareTo("vert")) neopixelWrite(RGB_BUILTIN, 9, 106, 9); 
    else if (!color.compareTo("blanc")) neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
    else if (!color.compareTo("jaune")) neopixelWrite(RGB_BUILTIN, 255, 255, 0); 
    else if (!color.compareTo("gris")) neopixelWrite(RGB_BUILTIN, 206, 206, 206); 
    else if (!color.compareTo("violet")) neopixelWrite(RGB_BUILTIN, 128, 0, 128); 
    else if (!color.compareTo("noir")) neopixelWrite(RGB_BUILTIN, 30, 5, 0); 
    else neopixelWrite(RGB_BUILTIN, 255, 0, 0); // erreur rouge 
}


/**
@brief fonction de sécurité dans le setup 
    allume la LED en rouge et bloque tout
*/
void stopSetup(String message) {
    setLED("rouge"); 
    Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.print("❌ ERREUR CRITIQUE : ");
    Serial.println(message);
    Serial.println("!!! SYSTÈME HALTÉ (BOUCLE INFINIE)");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    
    while(true) {
        // Optionnel : faire clignoter la LED rouge pour attirer l'attention
        setLED("rouge"); delay(500); setLED("blanc");  delay(500);
        Serial.println("  ==> mode panic ...");
    }
}

/**
 * @brief synchronise la date et l heure de l horloge PC<=>Esp32 
 */
bool syncDateTime() {
    struct tm tm_compile;
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
