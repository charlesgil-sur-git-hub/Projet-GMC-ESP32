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
    Preferences prefs;

    /**
      @brief : Parametres de connexion par Box en mode client
       CONFIGURATION PREMIER LANCEMENT : voir le SSID du wifi et le Pwd est la fin du SSID
        wifi : code SSID : "SSID_GMC_PASS_1234XXXX" , Pwd : "1234XXXX"
       allez sur : http://192.168.4.1/config
    */
    String boxSsid;
    String boxPwd;

    //! URL en mode client sur un Cloud (type alwaysdata ou autre)
    //!   ex : ""http://btscielinfo.alwaysdata.net/cloud/index.php" 
    String boxCloudUrl;


     /**
      @brief : Parametres de connexion par AP en mode serveur
      CONFIGURATION PREMIER LANCEMENT : voir le SSID du wifi et le Pwd est la fin du SSID
        wifi : code SSID : "SSID_GMC_PASS_1234XXXX" , Pwd : "1234XXXX"
       allez sur : http://192.168.4.1/config
    */
    String apSsid;
    String apPwd;

    /** @brief : params autres
    */
    int frequenceDesMesures;
    String modeSoloOuCluster;

public:
    Conf();
    
    // Charge les données depuis la Flash (NVS)
    bool begin();
    
    // Sauvegarde les données
    void save (String boxSsid, String boxPassword, String boxClourdUrl, 
                String apSsid, String apPassword, int freq, String mode);
    
    // Connexions AP (serveur) et/ou Box (client)
    
    String getBoxSSID() const { return boxSsid; }
    String getBoxPassword() const { return boxPwd; }
    String getBoxCloudUrl() { return boxCloudUrl; }

    String getApSSID() const { return apSsid; }
    String getApPassword() const { return apPwd; }

    int getFrequenceMesures() const { return frequenceDesMesures; }
    String getMode() const { return modeSoloOuCluster; }
    

    //! NVS a la place de DAO
    void saveDerniereMesure(int temp, bool alerte);
    int getLastTemp();
    bool getLastAlerte();


    // Réinitialisation d'usine
    void factoryReset();
};

#endif