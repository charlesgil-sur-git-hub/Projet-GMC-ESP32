/**
 * \brief classe acces base de donées SQLITe
 *
 * \file : DaoGMC.cpp
 * \date : 2026
 *
 Note: Vous devrez installer et inclure la librairie SQLite3 pour ESP32
 Exemple: #include <SQLite3.h>
 */
// DaoGMC.cpp
#include "daoGmc.h"


#define __debug


/**
    ************  table MESURE ************* 
*/


/**
 * @brief Sauve dans la base Sqlite la mesure
 * @param valeur_tdc : mesure de la temperatur en Dixiemes de Celsius
 */
bool DaoGMC::accederTableMesure_ecrireUneMesure (int valeur_tdc) {
    String sql = "INSERT INTO mesures (valeur_tdc) VALUES (" + String(valeur_tdc) + ");";
    #ifdef __debug
      Serial.print("ecrireUneMesure  sql: [");Serial.print(sql);Serial.println("]");
    #endif
    return (this->execute(sql.c_str()));
}

/**
 * @brief Lire dans la base Sqlite plusieurs mesures
 * @param limit : nombre de mesures à lire
 * @return std::vector<Mesure> : la liste des objets créés
 */
std::vector<Mesure> DaoGMC::accederTableMesure_lireDesMesures(unsigned short int limit) {
    std::vector<Mesure> listeObjetsMesure;
    
    // 1. Préparation de la requête
    String sql = "SELECT id_mesure, date_creation, valeur_tdc ";
    sql       += "FROM mesures ORDER BY date_creation DESC LIMIT " + String(limit) + ";";
    
    #ifdef __debug
        Serial.print("SQL Query: "); Serial.println(sql);
    #endif

    sqlite3_stmt *res;
    
    // 2. Exécution
    if (sqlite3_prepare_v2(this->getDb(), sql.c_str(), -1, &res, NULL) == SQLITE_OK) {
        
        while (sqlite3_step(res) == SQLITE_ROW) {
            // Lecture directe des colonnes SQL
            int id_mesure     = sqlite3_column_int(res, 0);
            const char* date_creationCstr = (const char*)sqlite3_column_text(res, 1);
            String date_creation   = (date_creationCstr != NULL) ? String(date_creationCstr) : "N/A";
            int valeur_tdc = sqlite3_column_int(res, 2);
            

            // 3. Instanciation de l'objet Mesure
            // On crée l'objet directement avec les données lues
            Mesure laMesure(id_mesure, date_creation, valeur_tdc);

            // 4. Ajout dans le vecteur
            listeObjetsMesure.push_back(laMesure);
        }
    } else {
        Serial.print("Erreur SQL Prepare: ");
        Serial.println(sqlite3_errmsg(this->getDb()));
    }

    // 5. Nettoyage
    sqlite3_finalize(res);

    return listeObjetsMesure;
}



/**
* @brief creation de la table mesures si existe pas
*/
bool DaoGMC::accederTableMesure_Creer() {
 
    /**
     * @brief creation de la table mesures si existe pas
     */
    String sql  = "CREATE TABLE IF NOT EXISTS mesures (";
    sql += "id_mesure INTEGER PRIMARY KEY AUTOINCREMENT, ";
    sql += "date_creation TEXT DEFAULT (datetime('now', 'localtime')), ";
    sql += "valeur_tdc INTEGER";
    sql += ");";
    return execute(sql.c_str());
}

 
    


/**
    ************************* 
*/

/**
* @brief begin : ouvre la base et creation des tables
*/
bool DaoGMC::begin() {
  bool  ret = false;

  //! Si erreur on quitte
  if (!LittleFS.begin(true)) 
    return false;

  //! Si erreur on quitte
  sqlite3_initialize();
  if (sqlite3_open(dbName, &db) != SQLITE_OK) 
    return false;

  /**
  * @brief creation de la table mesures si existe pas
  */
  ret = this->accederTableMesure_Creer();

  return ret;
}

/**
* @brief execute : execute une requete
* @param nom de la base
*/
bool DaoGMC::execute(const char* sql) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

/**
@brief close : ferme la conexion a la base
*/
void DaoGMC::close() {
    if (db) 
      sqlite3_close(db);
}


/**
 * @brief constructeur
 */
DaoGMC::DaoGMC(const char* name) : dbName(name), db(nullptr) 
{
 
}

DaoGMC::~DaoGMC() {
    close(); // On appelle close() automatiquement à la fin de vie de l'objet
}
