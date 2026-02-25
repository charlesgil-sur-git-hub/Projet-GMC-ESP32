/**
 * \brief classe de traitement au debut (on)
			de la fonction setup
 *
 * \file : setup_on.cpp
 * \date : 2026
 *
 */
#include "setup_on.h"

/**
 * @brief synchronise l horloge
 */
bool SetupOn::synchroniserHorlogeAuSetup() {
    struct tm tm_compile;
	//Serial.println("Synchronisation horloge ... ");  
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

/**
 * @brief preparer le moniteur serie au debut
 */
void SetupOn::preparerMoniteurAuSetup(Stream &SerialHS) {
    // 1. On informe l'utilisateur que le système démarre
    SerialHS.print("Initialisation du moniteur serie ");  
    
    uint32_t startTime = millis();
    
    // 2. Temporisation de confort (5 secondes)
    // On affiche des points pour montrer que l'ESP32 n'est pas "gelé"
    while ((millis() - startTime < 5000)) {
        delay(500);
        SerialHS.print(".");
    }

    // 3. On ne teste plus (!SerialHS) ici pour éviter l'erreur de compilation,
    // car Stream ne supporte pas l'opérateur '!'.
    // Si tu as besoin d'une attente réelle, elle se fait dans le .ino.

    SerialHS.println(" OK !");  
    SerialHS.println("--- Systeme Pret ---");
}
