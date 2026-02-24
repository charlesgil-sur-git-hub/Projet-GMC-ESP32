/**
 * \brief classe acces base de donées sans SQLITe mais Preferences
 *
 * \file : DaoGMC.cpp
 * \date : 2026
 *
 Note: utilisation de Preferences et pas de Sqlite
 */




#include "daoGmc.h"
#include <time.h> // Indispensable pour manipuler le temps

// Le constructeur reste vide car Preferences ne gère pas de fichier physique 'path'
DaoGMC::DaoGMC(const char* path) {}

bool DaoGMC::begin() {
    // On teste si on arrive à ouvrir le namespace au démarrage
    if (prefs.begin(_namespace, false)) {
        prefs.end(); // On referme tout de suite, on ouvrira à la demande
        return true;
    }
    return false;
}


/**
 * \brief Extrait la valeur : soit depuis du SQL, soit depuis une String brute
 */
int DaoGMC::extractionValeur(String query) {
    query.toUpperCase();

    // Si c'est du SQL "VALUES (123)"
    if (query.indexOf("VALUES") != -1) {
        int start = query.lastIndexOf('(') + 1;
        int end = query.lastIndexOf(')');
        if (start > 0 && end > start) {
            return query.substring(start, end).toInt();
        }
    }
    
    // Sinon, on extrait juste les chiffres présents dans la chaîne
    String numericPart = "";
    for (size_t i = 0; i < query.length(); i++) {
        if (isDigit(query[i])) numericPart += query[i];
    }

    return (numericPart.length() > 0) ? numericPart.toInt() : 0;
}
    

/**
 * \brief Intercepteur de requêtes SQL
 * On extrait la valeur numérique de la chaîne de caractères SQL
 */
 bool DaoGMC::execute(const char* sql) {
    String query = String(sql);
    query.toUpperCase();
    
    if (query.indexOf("INSERT INTO MESURES") != -1) {
        // ... (extraction de la valeur comme avant) ...
        int valeur = extractionValeur(query); 

        // Capturer l'heure actuelle de l'horloge interne (calée au setup)
        time_t maintenant = time(NULL); 

        prefs.begin(_namespace, false);
        int idx = prefs.getInt("idx", 0);

        // On stocke la valeur ET l'heure
        prefs.putInt(("v" + String(idx)).c_str(), valeur);
        prefs.putLong(("t" + String(idx)).c_str(), (long)maintenant); // Stockage du timestamp

        // On avance l'index (0 à 119 pour 1h de mesures)
        prefs.putInt("idx", (idx + 1) % 120);
        
        int count = prefs.getInt("count", 0);
        if (count < 120) prefs.putInt("count", count + 1);

        prefs.end();
        return true;
    }
    return true;
}

 

/**
 * \brief Méthode métier pour écrire
 */
bool DaoGMC::accederTableMesure_ecrireUneMesure(int valeur_tdc) {
    // On recrée la chaîne SQL que ton programme original attendait
    String sql = "INSERT INTO mesures (valeur_tdc) VALUES (" + String(valeur_tdc) + ");";
    return this->execute(sql.c_str());
}

/**
 * \brief Méthode métier pour lire
 * On retourne un vector pour rester compatible avec ton interface Web
 */
 std::vector<Mesure> DaoGMC::accederTableMesure_lireDesMesures(unsigned short int limit) {
    std::vector<Mesure> liste;
    prefs.begin(_namespace, true);
    
    int total = prefs.getInt("count", 0);
    int idxActuel = prefs.getInt("idx", 0);
    int aLire = (limit < total) ? limit : total;

    for (int i = 0; i < aLire; i++) {
        // On remonte le temps en partant de l'index actuel
        int targetIdx = (idxActuel - 1 - i + 120) % 120;
        
        int val = prefs.getInt(("v" + String(targetIdx)).c_str());
        time_t t = (time_t)prefs.getLong(("t" + String(targetIdx)).c_str());

        // Conversion du timestamp en "HH:MM:SS"
        struct tm * tm_info = localtime(&t);
        char bufferDateCreation[30]; 
        // 2. Change le format ( %d/%m/%Y pour la date, %H:%M:%S pour l'heure)
        strftime(bufferDateCreation, 30, "%d/%m/%Y %H:%M:%S", tm_info);

        liste.push_back(Mesure(targetIdx, String(bufferDateCreation), val));
    }
    
    prefs.end();
    return liste;
}



bool DaoGMC::accederTableMesure_Creer() { 
    Serial.println("DAO: Simulation de création de table OK (Preferences prête)");
    return true; 
}
