/**
 * @brief   Declaration de la classe modele pour 
				gerer le web et la page web
 * @file    net.h
 * @author  cgil 
 * @version 1.1 (Optimisée pour Preferences)
 * @date    fev 2026
 */

#ifndef NET_H
#define NET_H

#include <WebServer.h>
#include <LittleFS.h>

#include "dao.h" 
#include "conf.h"
#include "dbg.h"

// On indique au compilateur que l'objet dao 
// est défini dans le fichier principal
extern Dao* dao; 

class Net {
public:
    Net(WebServer&, Conf*);
    bool begin();
    void setupNetwork(); //! Wifi
    void setupRoutes();
    void haltSystem(); // bloquer le système en cas d'erreur

private:
    WebServer& _webServer;
    Conf* _conf; // On stocke une référence à la config
    
    // Handlers pour les requêtes
    void handleRoot();      // Pour afficher la page d'accueil
    void handleGetData();  // Pour renvoyer le JSON des mesures
    
    bool handleFileRead(String path);
    String getContentType(String filename);
};

#endif
