"""
/**
 * @brief   Programme client qui interroge un module gmc (ESP32)
 * @file   client.py
 * @author	cgil 
   @version	1.0
 * @date    mars 2026
 *
 * @details :
 *       le client interroge http://[IP_ESP32]/api/history_client  
 *
 *  la reponse en JSON
    [
      {"temp": 215, "date": "14:30"},
      {"temp": 218, "date": "14:15"},
      ... 118 autres mesures ...
    ]
 */
"""

import requests
import time

ESP32_IP = "192.168.1.16"

 

def collecter_donnees():
    try:
        print("Récupération de l'historique, module GMC sur ESP32 S3...")
        response = requests.get(f"http://{ESP32_IP}/api/history_client", timeout=5)
        
        if response.status_code == 200:
            donnees = response.json()
            if not donnees:
                print("⚠️ L'ESP32 a répondu, mais l'historique est VIDE.")
            else:
                print(f"\n✅ Reçu {len(donnees)} mesures.")
                
                # Affichage de la liste simplement
                for mesure in donnees:
                    # ON UTILISE LES CLÉS RÉELLES : 'temp' et 'date'
                    valeur = mesure['temp'] / 10  # Conversion (ex: 215 -> 21.5)
                    horodatage = mesure['date']
                    print(f"Date: {horodatage} | Valeur: {valeur}°C")
        
    except Exception as e:
        print(f"❌ Erreur de connexion : {e}")

# Le programme interroge l'ESP toutes les 30 s
while True:
    collecter_donnees()
    # time.sleep(1800) # 1800 secondes = 30 min
    time.sleep(30)
