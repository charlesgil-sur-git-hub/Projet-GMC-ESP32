/**
 * @brief   Declaration de la classe modele pour generer la page web
 * @file    webManager.h
 * @author  cgil 
 * @version 1.1 (Optimisée pour Preferences)
 * @date    fev 2026
 */
#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <WebServer.h>
#include <LittleFS.h>
#include "daoGmc.h" // Ajouté pour que le manager connaisse la DAO
#include "ConfigManager.h"

// On indique au compilateur que l'objet dao est défini dans le fichier principal
extern DaoGMC* dao; 

class WebManager {
public:
    WebManager(WebServer& server, ConfigManager* config);
    bool begin();
    void setupNetwork(); //! Wifi
    void setupRoutes();

private:
    WebServer& _webServer;
    ConfigManager* _configManager; // On stocke une référence à la config
    
    // Handlers pour les requêtes
    void handleRoot();      // Pour afficher la page d'accueil
    void handleGetData();  // Pour renvoyer le JSON des mesures
    
    bool handleFileRead(String path);
    String getContentType(String filename);
};

#endif
