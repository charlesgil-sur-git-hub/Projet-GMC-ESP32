/**
 * \brief classe acces base de données type SQLITe
			en utilisant "Preferences" ou NVS3
 *
 * \file : dao.h
 * \date :fev 26
 * \author : cgil
 *
 Note: on supprime Sqlite et ==> preferences
 
 */

#ifndef DAO_H
#define DAO_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "mesure.h"

class Dao {
private:
    Preferences prefs;
    const char* _namespace = "gmc_storage";

    /**
    @brief On extrait la valeur numérique de la chaîne de caractères SQL
    */
    int extractionValeur (String query); 


public:
    Dao(const char* path);
    bool begin();
    
    // Intercepteur de requêtes SQL
    bool execute(const char* sql);

    // Méthodes métier (signatures identiques à ton code d'origine)
    bool accederTableMesure_ecrireUneMesure(int valeur_tdc);
    std::vector<Mesure> accederTableMesure_lireDesMesures(unsigned short int limit);
    bool accederTableMesure_Creer();
};



#endif
