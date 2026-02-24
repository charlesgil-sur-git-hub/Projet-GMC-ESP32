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

#include "mesure.h"

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
    * @brief lit un ensemble de mesures
    *       et stocke dans une liste d'objects 'Mesure'
    * @param nombre de mesures
    */
    std::vector<Mesure> accederTableMesure_lireDesMesures (unsigned short int limit);

    /**
    * @brief sauve une mesure
    * @param valeur de la mesure
    */
    bool accederTableMesure_ecrireUneMesure (int valeur);


    /**
    * @brief creation de la table mesures si existe pas
    */
    bool accederTableMesure_Creer();
    
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
     bool execute(const char* sql);

    
        
    //!-----  Accesseurs--------------
    /**
    * @brief accesseurs
    */
    inline String getDbName() { return String(dbName); }
};

#endif
