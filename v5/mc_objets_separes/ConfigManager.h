/**
 * @brief   Classe de gestion de toute la partie 
 *                    configuration de l ESP32
 * @file    	ConfigManager.h
 * @author	cgil 
   @version	1.0
 * @date    fev 2026
 */
 
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
private:
    Preferences _prefs;
    String _ssid;
    String _password;
    int _freqMesure;
    String _mode;

public:
    ConfigManager();
    
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