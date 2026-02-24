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

/**
 * @brief constructeur
 */
DaoGMC::DaoGMC(const char* name) : dbName(name), db(nullptr) 
{
 
}


/**
* @brief begin : ouvre la base et creation des tables
*/
bool DaoGMC::begin() {
    //! Si erreur on quitte
    if (!LittleFS.begin(true)) 
      return false;

    //! Si erreur on quitte
    sqlite3_initialize();
    if (sqlite3_open(dbName, &db) != SQLITE_OK) 
      return false;

    /**
     * @brief : Pas d erreur au begin ==> donc on peut acceder
     */

    /**
     * @brief creation de la table mesures si existe pas
     */
    const char* sql = "CREATE TABLE IF NOT EXISTS mesures (id INTEGER PRIMARY KEY AUTOINCREMENT, valeur REAL, date TEXT);";
    
    return execute(sql);
}

void DaoGMC::close() {
    if (db) 
      sqlite3_close(db);
}

DaoGMC::~DaoGMC() {
    close(); // On appelle close() automatiquement à la fin de vie de l'objet
}
