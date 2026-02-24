

/**
*  @brief : Programme de traitement d'un Module Connecté ESP32
* 
*            Mise en oeuvre de classes en fichiers separes
* 
*
*   
*  @author :cgil - @2026
*  @version : v3
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

*
*
* @details mode d'emploi serveur Web
CONFIGURATION DU MODE RESEAU :
         - Module en réseau étoile   AVEC SCMC
         - Module en solo            SANS SCMC


 * En mode Solo :
     WiFi : Votre PC doit maintenant se connecter au réseau WiFi "SSID_GMC_MC01".
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
     CONFIGURATION DU MODE RESEAU :
         - Module en réseau étoile   AVEC SCMC
         - Module en solo            SANS SCMC
*/






//! --- Configuration en Réseau avec cluster  ---
const char* ssid_gmc_scmc = "SSID_GMC_SCMC"; // Le MC se connecte au WiFi du SCMC
const char* pass_scmc= "PWD_SCMC";


//! --- Configuration en solo WiFi (Mode Indépendant / Gateway) ---
const char* ssid_gmc_mc01 = "SSID_GMC_MC01";
const char* pass_mc01 = "PWD_MC01";




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

#define IS_SOLO true


// Objets globaux via pointeurs
WebServer server(80);
DaoGMC* dao = nullptr;
WebManager* webManager = nullptr;
SetupOn* so = nullptr;


/**
* Fonction utilitaire pour simplifier les couleurs
*/
void setLED(uint8_t r, uint8_t g, uint8_t b) {
   neopixelWrite(RGB_BUILTIN, r, g, b);
}


void setup() {
    // --- ÉTAPE 1.1 : ORANGE (BOOT) ---
    setLED(128, 40, 0);
    delay(2000); // Pour voir la couleur

    //! --- ÉTAPE 1.2 : preparer le moniteur serie
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && (millis() - start < 3000));
    Serial.println("\n--- DEMARRAGE SYSTEME GMC ---");
    setLED(192, 40, 0);
    delay(2000); 

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
   setLED(0, 0, 128);
   Serial.println("Initialisation DAO (Preferences)...");
   dao = new DaoGMC("/littlefs/gmc.db");
   if (dao->begin()) {
       Serial.println("DAO Init: OK");
   }
   delay(2000); // Pour voir la couleur


   // --- ÉTAPE 3 : VIOLET (RESEAU) ---
   setLED(128, 0, 128);
   if (IS_SOLO) {
       Serial.println("Mode SOLO (Point d'accès)...");
       WiFi.softAP("SSID_GMC_MC01", "PWD_MC01");
   } else {
       Serial.println("Mode CLUSTER (Connexion SCMC)...");
       WiFi.begin("SSID_GMC_SCMC", "PWD_SCMC");
       while (WiFi.status() != WL_CONNECTED) {
           setLED(64, 0, 64); delay(200);
           setLED(0, 0, 0); delay(200);
       }
   }
   Serial.println("Reseau OK.");
   delay(500); // Pour voir la couleur


   // --- ÉTAPE 4 : INITIALISATION WEB ---
   webManager = new WebManager(server);
   // AJOUTE CETTE LIGNE ICI :
   if (webManager->begin()) {
       Serial.println("WebManager : LittleFS chargé avec succès !");
   } else {
       Serial.println("WebManager : Erreur critique LittleFS !");
       exit(-2);
   }
   webManager->setupRoutes();
   server.begin();


   // --- ÉTAPE 5 : FLASH VERT (PRÊT) ---
   setLED(0, 128, 0);
   delay(1000);
  
   // On laisse une couleur de fond pour savoir dans quel mode on tourne
   if (IS_SOLO) setLED(30, 5, 0); // Orange très faible
   else setLED(0, 0, 30);         // Bleu très faible
}


void loop() {
   server.handleClient();


   static unsigned long lastUpdate = 0;
   if (millis() - lastUpdate > 30000) { // ttes les 30s
       lastUpdate = millis();
      
       // --- FLASH VERT RAPIDE (VIE) ---
       setLED(0, 255, 0);
      
       int val = random(180, 260);
       if(dao->accederTableMesure_ecrireUneMesure(val)) {
           Serial.printf("Mesure stockée : %d\n", val);
       }
      
       delay(100);
       // Retour à la couleur de veille
       if (IS_SOLO) setLED(30, 5, 0);
       else setLED(0, 0, 30);
   }
}




