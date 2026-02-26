/**
 * @brief   Classe de gestion de toute la partie 
 *                    configuration de l ESP32
 * @file    	conf.h
 * @author	cgil 
   @version	1.0
 * @date    fev 2026
 */
 
#ifndef CONF_H
#define CONF_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @class conf
 * @brief Gère la configuration persistante du module (JSON sur LittleFS).
 * * GUIDE AJOUT PARAMÈTRE :
 * 1. Déclarer la variable : String _monParam;
 * 2. Créer le Getter : String getMonParam() { return _monParam; }
 * 3. Créer le Setter : void setMonParam(String v) { _monParam = v; }
 * 4. Mettre à jour load() : _monParam = doc["monParam"] | "defaut";
 * 5. Mettre à jour save() : doc["monParam"] = _monParam;
 */
class Conf {
private:
    Preferences _prefs;
    String _ssid;
    String _password;
    int _freqMesure;
    String _mode;

public:
    Conf();
    
    // Charge les données depuis la Flash (NVS)
    void begin();
    
    // Sauvegarde les données
    void save(String ssid, String pass, int freq, String mode);
    
    // Getters (pour lire les valeurs)
    String getSSID() const { return _ssid; }
    String getPassword() const { return _password; }
    int getFreq() const { return _freqMesure; }
    String getMode() const { return _mode; }
    
    // Réinitialisation d'usine
    void factoryReset();
};

#endif