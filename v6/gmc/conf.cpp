/**
 * @brief   Classe de gestion de toute la partie 
 *                    configuration des parametres de l ESP32
 * @file    	conf.h
 * @author	cgil 
   @version	1.0
 * @date    fev 2026
 */
 
#include "conf.h"


Conf::Conf() {}

bool Conf::begin() {
    this->prefs.begin("settings", true); // Mode lecture seule

    // 1. On récupère l'ID unique (MAC)
    //          par defaut : '1234XXXX'
    uint64_t chipId = ESP.getEfuseMac();
    String uniqueId = String((uint32_t)(chipId >> 32), HEX);
    uniqueId.toUpperCase();
    //String chipIdStr = uniqueId.substring(uniqueId.length() - 4);
    //String defaultSSID = "SSID_GMC_MC_" + uniqueId.substring(uniqueId.length() - 4);
    String defaultApSSID = "";
    defaultApSSID += "SSID_GMC_PASS_1234";
    defaultApSSID += uniqueId;

    
    // 2. On charge le SSID depuis les préférences, 
    // MAIS on passe notre identifiant unique comme valeur par défaut !
     this->boxSsid = prefs.getString("boxSsid", "ssid_box_a_renseigner");
    this->boxPwd = prefs.getString("boxPwd", "pwd_box_a_renseigner");

    /**
      @brief URL en mode client sur un Cloud (type alwaysdata ou autre)
    */    
    this->boxCloudUrl = prefs.getString("boxCloudUrl", "http://btscielinfo.alwaysdata.net/cloud/index.php");

    this->apSsid = prefs.getString("apSsid", defaultApSSID);
    this->apPwd = prefs.getString("apPwd", "1234" + uniqueId);
   
    this->frequenceDesMesures = prefs.getInt("frequenceDesMesures", 30);
    this->modeSoloOuCluster = prefs.getString("modeSoloOuCluster", "modeSolo");


    // Si pour une raison X ou Y (après un reset), les valeurs sont vides ou corrompues
     if (this->boxSsid.length() < 8) 
        this->boxSsid = "ssid_box_a_renseigner";
    if (this->boxPwd.length() < 8 ) 
        this->boxPwd = "pwd_box_a_renseigner";
     if (this->boxCloudUrl.length() < 8 ) 
        this->boxCloudUrl = "http://btscielinfo.alwaysdata.net/cloud/index.php";

    if (this->apSsid.length() < 8) 
        this->apSsid = defaultApSSID;
    if (this->apPwd.length() < 8) 
        this->apPwd = "1234" + uniqueId;
   
    
    if (this->frequenceDesMesures == 0) 
        this->frequenceDesMesures = 30;  //! Mesure ttes les 30 s
    if (this->modeSoloOuCluster != "solo" && this->modeSoloOuCluster != "cluster") 
        this->modeSoloOuCluster = "solo"; 
    this->prefs.end();

    //! on Resauve pour si on a modifie
    this->save (this->boxSsid, this->boxPwd, this->boxCloudUrl, 
                    this->apSsid, this->apPwd, 
                    this->frequenceDesMesures, this->modeSoloOuCluster);

    return true;
}

void Conf::save(String box_ssid, String box_pwd, String box_cloud_url,
        String ap_ssid, String ap_pwd, 
        int frequence_mesures, String mode_ou_cluster) {
    this->prefs.begin("settings", false); // Mode écriture
    
    this->prefs.putString("boxSsid", box_ssid);
    this->prefs.putString("boxPwd", box_pwd);
    this->prefs.putString("boxCloudUrl", box_cloud_url);

    this->prefs.putString("apSsid", ap_ssid);
    this->prefs.putString("apPwd", ap_pwd);
  

    this->prefs.putInt("frequenceDesMesures", frequence_mesures);
    this->prefs.putString("modeSoloOuCluster", mode_ou_cluster);
    
    this->prefs.end();
    
    // On met à jour les variables locales pour éviter de redémarrer si pas nécessaire
    this->boxSsid = box_ssid;
    this->boxPwd = box_pwd;
    this->boxCloudUrl = box_cloud_url;
   
    this->apSsid = ap_ssid;
    this->apPwd = ap_pwd;
  
    this->frequenceDesMesures = frequence_mesures;
    this->modeSoloOuCluster = mode_ou_cluster;

    
}

void Conf::factoryReset() {
    prefs.begin("settings", false);
    prefs.clear();
    prefs.end();
    Serial.println("Config effacée. Redémarrage...");
    delay(1000);
    ESP.restart();
}