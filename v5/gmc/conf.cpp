/**
 * @brief   Classe de gestion de toute la partie 
 *                    configuration des parametres de l ESP32
 * @file    	conf.h
 * @author	cgil 
   @version	1.0
 * @date    fev 2026
 */
 
#include "conf.h"


//! --- DEFAUT  ---
const char* ssid_default = "SSID_GMC_PASS_1234XXXX";
const char* pass_default= "1234XXXX";


//! --- Configuration en Réseau avec cluster  ---
const char* ssid_gmc_scmc = "SSID_GMC_SCMC"; // Le MC se connecte au WiFi du SCMC
const char* pass_scmc= "PWD_SCMC";

//! --- Configuration en initlo WiFi (Mode Indépendant / Gateway) ---
const char* ssid_gmc_mc01 = "SSID_GMC_MC01";
const char* pass_mc01 = "PWD_MC01";




Conf::Conf() {}

void Conf::begin() {
    _prefs.begin("settings", true); // Mode lecture seule

    // 1. On récupère l'ID unique (6 derniers caractères de la MAC)
    //          par defaut : '1234XXXX'
    uint64_t chipId = ESP.getEfuseMac();
    String uniqueId = String((uint32_t)(chipId >> 32), HEX);
    uniqueId.toUpperCase();
    String chipIdStr = uniqueId.substring(uniqueId.length() - 4);
    //String defaultSSID = "SSID_GMC_MC_" + uniqueId.substring(uniqueId.length() - 4);
    String defaultSSID = "SSID_GMC_PASS_1234" + uniqueId;
    
    // 2. On charge le SSID depuis les préférences, 
    // MAIS on passe notre identifiant unique comme valeur par défaut !
    _ssid = _prefs.getString("ssid", defaultSSID);

    _password = _prefs.getString("password", "1234" + uniqueId);
    _freqMesure = _prefs.getInt("freq", 30);
    _mode = _prefs.getString("mode", "initlo");

    // Si pour une raiinitn X ou Y (après un reset), le mode est vide ou corrompu
    if (_mode != "initlo" && _mode != "cluster") {
        _mode = "initlo"; 
    }
    if (_ssid.length() == 0) {
        _ssid = defaultSSID;
    }
    if (_password.length() == 0) {
        _password = chipIdStr;
    }
    if (_freqMesure == 0) {
        _freqMesure = 30;
    }

    _prefs.end();

    //! on Resauve pour si on a modifie
    this->save(_ssid, _password, _freqMesure, _mode);
}

void Conf::save(String ssid, String pass, int freq, String mode) {
    _prefs.begin("settings", false); // Mode écriture
    
    _prefs.putString("ssid", ssid);
    _prefs.putString("password", pass);
    _prefs.putInt("freq", freq);
    _prefs.putString("mode", mode);
    
    _prefs.end();
    
    // On met à jour les variables locales pour éviter de redémarrer si pas nécessaire
    _ssid = ssid;
    _password = pass;
    _freqMesure = freq;
    _mode = mode;
}

void Conf::factoryReset() {
    _prefs.begin("settings", false);
    _prefs.clear();
    _prefs.end();
    Serial.println("Config effacée. Redémarrage...");
    delay(1000);
    ESP.restart();
}