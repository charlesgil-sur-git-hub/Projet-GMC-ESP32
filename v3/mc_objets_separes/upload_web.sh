#!/bin/bash

# --- CONFIGURATION DYNAMIQUE ---
DIR_ESP32_ROOT="$HOME/.arduino15/packages/esp32"

# 1. On cherche l'exécutable mklittlefs
MKLITTLEFS_BIN=$(find "$DIR_ESP32_ROOT" -name "mklittlefs" -type f | head -n 1)

# 2. On cherche l'exécutable esptool
ESPTOOL_BIN=$(find "$DIR_ESP32_ROOT" -name "esptool" -type f | head -n 1)

# --- VÉRIFICATION DES OUTILS ---
if [ -z "$MKLITTLEFS_BIN" ] || [ -z "$ESPTOOL_BIN" ]; then
    clear
    echo "❌ ERREUR : Outils ESP32 introuvables."
    echo "MKLITTLEFS : ${MKLITTLEFS_BIN:-NON TROUVÉ}"
    echo "ESPTOOL    : ${ESPTOOL_BIN:-NON TROUVÉ}"
    echo "----------------------------------------------------------"
    exit 1
fi

# --- ÉTAPE 1 : SÉCURITÉ MONITEUR SÉRIE ---
clear
echo "=========================================================="
echo "          TRANSFERT DES FICHIERS WEB (LittleFS)           "
echo "=========================================================="
echo " Utilisateur : $USER"
echo " Adresse cible : 0x290000"
echo " Taille partition : 1441792 octets"
echo "----------------------------------------------------------"
echo ""
echo " ATTENTION : Fermez le MONITEUR SÉRIE de l'IDE Arduino !"
echo ""
echo "----------------------------------------------------------"
echo "Appuyez sur une touche quand c'est fait..."
read -n 1 -s

# --- ÉTAPE 2 : GÉNÉRATION DE L'IMAGE ---
echo ""
echo "Génération de l'image LittleFS..."
# On utilise la taille exacte reportée par ton ESP32 : 1441792
$MKLITTLEFS_BIN -c data -s 1441792 -p 256 data.bin

# --- ÉTAPE 3 : TÉLÉVERSEMENT ---
echo "Connexion à l'ESP32-S3..."
# Flashage à l'adresse 0x290000 avec vitesse stable
$ESPTOOL_BIN --chip esp32s3 --port /dev/ttyUSB0 --baud 115200 write_flash 0x290000 data.bin

# --- ÉTAPE 4 : VÉRIFICATION ---
if [ $? -eq 0 ]; then
    echo ""
    echo "----------------------------------------------------------"
    echo " ✅ SUCCÈS : Le site web est stocké à 0x290000 !"
    echo " Used LittleFS ne devrait plus être à 8192 au reboot."
else
    echo ""
    echo "----------------------------------------------------------"
    echo " ❌ ERREUR : Le téléversement a échoué."
    echo " Vérifiez que /dev/ttyUSB0 n'est pas utilisé."
fi

# --- ÉTAPE 5 : FIN ---
echo ""
echo "----------------------------------------------------------"
echo " TERMINE ! Vous pouvez rouvrir le Moniteur Série."
echo " Appuyez sur une touche pour quitter."
read -n 1 -s
