
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

#include <daoGmc.h>       //! Classe de gestion access BD


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
 *  \brief : classe NetworkManagerGMC
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
// CLASSE 3 : MeasureModel
// ======================================================
class MeasureModel {
public:
    static void save(DaoGMC &dao, float value) {
        String sql = "INSERT INTO mesures (valeur, date) VALUES (" + String(value) + ", datetime('now'));";
        dao.execute(sql.c_str());
    }

    static String getAllAsJSON(DaoGMC &dao, int limit = 10) {
        DynamicJsonDocument doc(2048);
        JsonArray root = doc.to<JsonArray>();
        sqlite3_stmt *res;
        String query = "SELECT * FROM mesures ORDER BY id DESC LIMIT " + String(limit) + ";";

        if (sqlite3_prepare_v2(dao.getDb(), query.c_str(), -1, &res, NULL) == SQLITE_OK) {
            while (sqlite3_step(res) == SQLITE_ROW) {
                JsonObject obj = root.createNestedObject();
                obj["id"] = sqlite3_column_int(res, 0);
                obj["temp"] = sqlite3_column_double(res, 1);
                obj["date"] = (const char*)sqlite3_column_text(res, 2);
            }
        }
        sqlite3_finalize(res);
        String output;
        serializeJson(doc, output);
        return output;
    }

    static String getLatestHTML(DaoGMC &dao) {
        String html = "<tr><th>ID</th><th>Valeur</th><th>Date</th></tr>";
        sqlite3_stmt *res;
        if (sqlite3_prepare_v2(dao.getDb(), "SELECT * FROM mesures ORDER BY id DESC LIMIT 5;", -1, &res, NULL) == SQLITE_OK) {
            while (sqlite3_step(res) == SQLITE_ROW) {
                html += "<tr><td>" + String(sqlite3_column_int(res, 0)) + "</td>";
                html += "<td>" + String(sqlite3_column_double(res, 1)) + " °C</td>";
                html += "<td>" + String((const char*)sqlite3_column_text(res, 2)) + "</td></tr>";
            }
        }
        sqlite3_finalize(res);
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
    String page = "<html><head><meta charset='UTF-8'><style>body{font-family:sans-serif;text-align:center;}table{margin:auto;border-collapse:collapse;}td,th{border:1px solid #333;padding:10px;}</style></head><body>";
    page += "<h1>Module Indépendant</h1><table border='1'>" + MeasureModel::getLatestHTML(dao) + "</table>";
    page += "<script>setTimeout(function(){location.reload();}, 5000);</script></body></html>";
    server.send(200, "text/html", page);
}

void handleJSON() {
    server.send(200, "application/json", MeasureModel::getAllAsJSON(dao));
}


/**
 * @brief setup du programme
 */
void setup() {
    pinMode(RGB_BUILTIN, OUTPUT); // Sur S3, c'est souvent une LED RGB
    digitalWrite(RGB_BUILTIN, HIGH); // On l'allume tout de suite
    
    Serial.begin(115200);
    // Attend que le moniteur série soit ouvert (max 10 secondes)
    uint32_t startTime = millis();
    while (!Serial && (millis() - startTime < 10000)) {
        delay(10);
    }
    Serial.println("\n--- 1. CONNEXION ETABLIE AVEC LE MONITEUR SERIE---");
    delay(2000); // Petite pause de confort
    Serial.println("--- 2. LANCEMENT DU SYSTEME ---");;
    
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
    

    server.on("/get_data", handleJSON);
    server.begin();
}



void loop() {
    server.handleClient();

    //! Simulateur qui enregistre toutes les 30 s des valeurs alea
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 30000) {
        MeasureModel::save(dao, random(180, 260) / 10.0);
        lastUpdate = millis();
    } 
}
