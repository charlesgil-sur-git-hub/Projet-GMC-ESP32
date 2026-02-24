/**
 * \brief classe acces base de donées SQLITe 
 *
 * \file : daoGmc.h
 * \date :fev 26
 * \author : cgil
 *
 Note: Vous devrez installer et inclure la librairie SQLite3 pour ESP32
 Exemple: #include <SQLite3.h>
 */


// DAOGMC.h
#ifndef DAOGMC_H
#define DAOGMC_H

#include <Arduino.h>

#include <sqlite3.h>
#include <LittleFS.h>
#include <FS.h> // Nécessaire pour la SD ou LittleFS

// Note: Vous devrez installer et inclure la librairie SQLite3 pour ESP32
// Exemple: #include <SQLite3.h>

/** 
 *  @brief : classe DaoGMC : gestion des acces à la base SQLITE
*/
class DaoGMC {
private:
    /**
    * @brief instance du curseur SQLITE
    * // SQLite3 db; // Décommenter après installation de la librairie
    */
    sqlite3 *db;

    /**
    * @brief nom de la base
    */
    const char* dbName;
    
   
public:
    /**
    * @brief constructeur parametrique
    * @param nom de la base
    */
    //! DaoGMC(const char* name) : dbName(name), db(nullptr) {}
    DaoGMC(const char* name);

    /**
     * destructeur
     */
    ~DaoGMC(); 

    /**
    * @brief constructeur parametrique
    * @param nom de la base
    */
    inline sqlite3* getDb() { return db; }

    

    /**
    * @brief begin : ouvre la base et creation des tables
    */
    inline bool begin_inline() {
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

    /**
    * @brief begin : ouvre la base et creation des tables
    */
    bool begin();

    /**
    * @brief close
    */
    void close();

    
    /**
    * @brief execute : execute une requete
    * @param nom de la base
    */
    inline bool execute(const char* sql) {
        char *zErrMsg = 0;
        int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            Serial.printf("SQL Error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }
        return true;
    }

        
    //!-----  Accesseurs--------------
    /**
    * @brief accesseurs
    */
    inline String getDbName() { return String(dbName); }
};

#endif
