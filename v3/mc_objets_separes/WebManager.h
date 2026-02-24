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

// On indique au compilateur que l'objet dao est défini dans le fichier principal
extern DaoGMC* dao; 

class WebManager {
public:
    WebManager(WebServer& server);
    bool begin();
    void setupRoutes();

private:
    WebServer& _server;
    
    // Handlers pour les requêtes
    void handleRoot();      // Pour afficher la page d'accueil
    void handleGetData();  // Pour renvoyer le JSON des mesures
    
    bool handleFileRead(String path);
    String getContentType(String filename);
};

#endif
