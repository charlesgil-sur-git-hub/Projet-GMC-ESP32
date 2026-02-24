
/**
 *  \brief : Programme de traitement d'un Module Connecté ESP32
 *  
 *            Mise en oeuvre de classes en fichiers separes
 *  
 * 
 *  \details: Programme en Objet
 *  
 * \author :cgil - @2026
 */

 /**
  * \details mode d'emploi
  * Vérifions si votre base de données se remplit bien.
    
    1 : Connectez votre PC/Smartphone au WiFi "SSID_GMC_MC01" et password "PWD_MC01"
    2 : Ouvrez votre navigateur 
        allez sur : http://192.168.4.1
        allez sur : http://192.168.4.1/get_data 

    3 : verifier le resultat en JSON
        [
          {"id":1, "temp":24.5, "hum":50, "date":"2026-02-06 11:45"},
          {"id":2, "temp":24.6, "hum":51, "date":"2026-02-06 11:46"}
        ]
 */
 
#include <WiFi.h>
#include <WebServer.h>

#include <ArduinoJson.h>

#include "daoGmc.h"         //! Classe de gestion access BD
#include "mesure.h"         //! objet mesure


/**
      CONFIGURATION DU MODE RESEAU :
          - Module en réseau étoile   AVEC SCMC
          - Module en solo            SANS SCMC
*/
/**
  \brief : Usage

  En mode Solo :
      WiFi : Votre PC doit maintenant se connecter au réseau WiFi "SSID_GMC_MC01".
      Accès : Dans votre navigateur, 
        tapez http://192.168.4.1 (l'IP par défaut d'un ESP32 en mode Point d'Accès).

      Double usage : * Si vous allez sur /, 
      vous voyez un joli petit tableau (le HTML est généré par l'ESP, 
          code très court donc rapide à téléverser).

        Si vous allez sur /get_data, vous avez toujours votre JSON. 
          Donc, si plus tard vous décidez d'allumer un SCMC, 
          ce dernier pourra toujours interroger le module sans rien changer !
*/

// --- Configuration en Réseau avec cluster  ---
const char* ssid_gmc_scmc = "SSID_GMC_SCMC"; // Le MC se connecte au WiFi du SCMC
const char* pass_scmc= "PWD_SCMC";

// --- Configuration en solo WiFi (Mode Indépendant / Gateway) ---
const char* ssid_gmc_mc01 = "SSID_GMC_MC01"; 
const char* pass_mc01 = "PWD_MC01";

  



/** 
 *  \brief : classe NetworkManagerGMC inline
 *  
 *  classe de gestion du réseau SOLO ou en CLUSTER
*/
class NetworkManagerGMC {
public:
    enum Mode { SOLO, CLUSTER };

    void setup(Mode mode, const char* ssid, const char* pass, IPAddress local, IPAddress gateway, IPAddress subnet) {
        if (mode == CLUSTER) {
            Serial.println("Mode: CLUSTER (STA)");
            WiFi.config(local, gateway, subnet);
            WiFi.begin(ssid, pass);
            while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
            Serial.println("\nConnecté. IP: " + WiFi.localIP().toString());
        } else {
            Serial.println("Mode: SOLO (AP)");
            WiFi.softAP(ssid, pass);
            Serial.println("AP IP: " + WiFi.softAPIP().toString());
        }
    }
};

// ======================================================
// CLASSE MesureModele
// ======================================================
class MesureModele {
public:
   
    static String getAllAsJSON(DaoGMC &dao, int limit = 10) {
        // 1. Récupération des données (via le DAO)
        std::vector<Mesure> liste = dao.accederTableMesure_lireDesMesures(limit);
   
        // 2. Préparation du document JSON
        // Pour un ESP32-S3, 1024 ou 2048 octets c'est "indolore"
        DynamicJsonDocument doc(2048); 
        JsonArray array = doc.to<JsonArray>();
    
        // 3. Mapping des objets Mesure vers le JSON
        for (const auto& m : liste) {
            JsonObject obj = array.createNestedObject();
            obj["id_mesure"]   = m.getIdMesure();
            obj["date_creation"] = m.getDateCreation();
            obj["valeur_tdc"] = m.getValeurTdc();
        }
        
        // 4. Sérialisation en String
        String output;
        serializeJson(doc, output);
        return output;
    }

   
    static String getLatestHTML(DaoGMC &dao) {
      // 1. On utilise le DAO pour récupérer les 5 derniers objets Mesure
      std::vector<Mesure> liste = dao.accederTableMesure_lireDesMesures(5);
      String valeur_tdcStr = "";
      
      // 2. On prépare l'entête du tableau
      String html = "<tr><th>ID</th><th>Date création</th><th>T° en celsius</th></tr>";
      
      // 3. Si la liste est vide, on peut mettre un message
      if (liste.empty()) {
          return "<tr><td colspan='3'>Aucune donnée enregistrée</td></tr>";
      }
  
      // 4. On boucle sur le vecteur pour construire les lignes
      for (const auto& m : liste) {
          html += "<tr>";
          html += "<td>" + String(m.getIdMesure()) + "</td>";
          html += "<td>" + m.getDateCreation() + "</td>";
          int entiereValeurTdc = m.getValeurTdc() / 100;
          int decimaleValeurTdc = m.getValeurTdc() % 100;
          valeur_tdcStr = String(entiereValeurTdc) + ",";
          valeur_tdcStr +=(decimaleValeurTdc < 10 ? "0" : "") + String(decimaleValeurTdc);
          html += "<td>" + String(valeur_tdcStr) + " °C</td>";
          html += "</tr>";
      }
    
      return html;
    }
};

  

// ======================================================
// INSTANCIATION & SERVEUR
// ======================================================

// CHOIX DU MODE ICI :
#define IS_SOLO true 

WebServer server(80);

NetworkManagerGMC net;

DaoGMC dao("/littlefs/gmc.db");


/**
 * @brief creation page web root
 */

void handleRoot() {
    // 1. On commence la page
    String page = "<html><head><meta charset='UTF-8'>";
    page += "<style>body{font-family:sans-serif;text-align:center;} ";
    page += "table{margin:auto;border-collapse:collapse;} ";
    page += "td,th{border:1px solid #333;padding:10px;} .empty{color:gray;font-style:italic;}";
    page += "</style></head><body>";
    page += "<h1>Module SCMC - Temps Réel</h1>";

    // 2. On récupère le contenu HTML généré par le modèle
    String tableauContent = MesureModele::getLatestHTML(dao);

    if (tableauContent == "") {
        page += "<p class='empty'>En attente de la première mesure...</p>";
    } else {
        page += "<table border='1'>" + tableauContent + "</table>";
    }

    // 3. Script d'auto-refresh (15 secondes)
    page += "<script>setTimeout(function(){location.reload();}, 15000);</script>";
    page += "</body></html>";

    server.send(200, "text/html", page);
}


//! version Pro
void handleJSON() {
    server.send(200, "application/json", MesureModele::getAllAsJSON(dao));
}


void synchroniserHeureCompilation() {
    struct tm tm_compile;
    // On récupère les macros de compilation __DATE__ et __TIME__
    // Format : "Feb  6 2026" et "12:34:56"
    if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm_compile)) {
        time_t t = mktime(&tm_compile);
        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        Serial.println(">>> Horloge ESP32 calée sur l'heure du PC !");
    }
}

/**
 * @brief setup du programme
 */
void setup() {
    //! Pour voir qu'on demarre bien...
    pinMode(RGB_BUILTIN, OUTPUT); digitalWrite(RGB_BUILTIN, HIGH); delay(1000);
    pinMode(RGB_BUILTIN, OUTPUT); digitalWrite(RGB_BUILTIN, LOW); delay(1000);
    pinMode(RGB_BUILTIN, OUTPUT); digitalWrite(RGB_BUILTIN, HIGH);
    
    Serial.begin(115200);
    Serial.println("Serial.begin(115200)...");

    // Attend que le moniteur série soit ouvert (max 10 secondes)
    uint32_t startTime = millis();
    while (!Serial && (millis() - startTime < 10000)) {
        delay(200);
        Serial.println("while (!Serial && (millis() - startTime < 10000)...");
    }
    Serial.println("\n--- 1. CONNEXION MONITEUR SERIE---");  delay(2000); 
    Serial.println("\n--- 2. LANCEMENT DU SYSTEME ... ");

    // 0 : On appelle la synchro AVANT de lancer la base de données
    synchroniserHeureCompilation();
    
    // 1. Database
    if (!dao.begin()) {
      Serial.println("!!! ");
      Serial.println("    DAO Error!");
      Serial.println("!!!");
      exit(-1);
    }

    // 2. Network
    IPAddress local(192,168,4,10);
    IPAddress gate(192,168,4,1);
    IPAddress sub(255,255,255,0);

    if (IS_SOLO) {
      /** 
       *  \brief : --- CONFIGURATION EN MODE POINT D'ACCÈS (GATEWAY) --- 
       */
      net.setup(NetworkManagerGMC::SOLO, "SSID_GMC_MC01", "PWD_MC01", local, gate, sub);

      // --- Routes ---
      server.on("/", handleRoot);
      
    } else {
      /** 
      *  \brief : --- CONFIGURATION EN MODE CLUSTER --- 
      */
      net.setup(NetworkManagerGMC::CLUSTER, "SSID_GMC_SCMC", "PWD_SCMC", local, gate, sub);
    }
    

    //! Version PRO : 
    //!   server.on("/get_data", handleJSON);
    //! Version Moderne : 
    server.on("/get_data", []() {
      String payload = MesureModele::getAllAsJSON(dao, 10);
      server.send(200, "application/json", payload);
    });

    server.begin();
}



void loop() {

    server.handleClient();
    
    //! Simulateur qui enregistre toutes les 30 s des valeurs alea
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 30000) {
       int valeur_tdc_random = random(18, 26) * 100;  //! * 100 pour celcius
       int valeur_tdc_centiemes_random = random(1, 9) * 10;  
       valeur_tdc_random += valeur_tdc_centiemes_random;
       if ( !dao.accederTableMesure_ecrireUneMesure (valeur_tdc_random))
         Serial.println("\n !! Erreur lors de la sauvegarde de la mesure en base");
        
       lastUpdate = millis();

    } 
}


//! ----------------- Pour Debug
/**
 *  Tests pour afficher des mesures
 */
 /*
void afficherListeMesures() {
  // 1. On récupère la liste des 10 dernières mesures
  std::vector<Mesure> mesMesures = dao.accederTableMesure_lireDesMesures(10);
  Serial.println("--- AFFICHAGE DES MESURES ---");
  // 2. On parcourt le vecteur (liste)
  // "m" devient tour à tour chaque objet Mesure contenu dans le vecteur
  for (const auto& m : mesMesures) {
    Serial.print("ID: "); Serial.print(m.getIdMesure());
    Serial.print(" | Date: "); Serial.print(m.getDateCreation());
    Serial.print(" | Temp C° : "); Serial.println(m.getValeurTdc());
  }
  Serial.println("-----------------------------");
}  
*/  
