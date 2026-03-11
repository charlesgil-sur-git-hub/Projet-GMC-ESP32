# ☢️ Projet GMC - ESP32 Control Center (V3)

Ce projet consiste en la création d'une interface de monitoring pour un compteur Geiger-Müller (GMC) utilisant un **ESP32**. 
L'architecture a été conçue pour être pédagogique, robuste et performante.

## 🚀 Évolution du Projet
* **V0 & V1** : Concepts de base et connectivité WiFi.
* **V2** : Tentative de stockage SQLite (abandonnée pour instabilité).
* **V3 ** : Architecture découplée avec DAO, stockage via `Preferences` et interface Web via `LittleFS`.
* **V4 (Actuelle)** : Config application par NVS, gestion erreurs, Wifi Hybride pour tests debug.

* ## 🛠 Guide de Développement (v5.0)

Ce projet est conçu comme un prototype modulaire. 
conf.h/.cpp	Gestion de la configuration JSON.
net.h/.cpp	Serveur Web, API REST et WiFi.
dao.h/.cpp	Accès aux données (Mesures).
dbg.h/.cpp	Outils de débogage.


### Comment ajouter un paramètre de configuration ?
Si vous voulez ajouter un identifiant de module (ex: `moduleID`) :
1. Dans `conf.h` : Ajoutez la variable privée et ses méthodes `get/set`.
2. Dans `conf.cpp` :
   - Dans `load()` : Ajoutez `_moduleID = doc["moduleID"] | "ESP32-DEV";`
   - Dans `save()` : Ajoutez `doc["moduleID"] = _moduleID;`
3. Dans l'interface Web : Ajoutez un champ `<input>` avec l'ID correspondant.


## 🏗️ Architecture Logicielle
Le code est structuré de manière modulaire pour séparer les responsabilités :

1. **DAO (Data Access Object)** : Gère le stockage circulaire des 120 dernières mesures (1 heure de données) dans la mémoire flash (NVS).
2. **net** : Gère le serveur HTTP, les routes JSON (`/api/status`, `/api/history`) et le service des fichiers statiques.
3. **Data/ (LittleFS)** : Contient l'interface frontend (HTML5, CSS3, JS) totalement isolée du code C++.



## 🛠️ Fonctionnalités de la V3
- **Monitoring Temps Réel** : Affichage de la température (ou TDC) avec mise à jour automatique.
- **Historique** : Buffer tournant de 120 points de données.
- **Horloge Synchronisée** : Synchronisation automatique de l'heure de l'ESP32 via le navigateur client (pas besoin de serveur NTP).
- **Contrôle Hardware** : Pilotage de la LED intégrée via l'interface Web.
- **Mode Hybride** : Le script JS permet un mode test local (PC) ou un mode réel (ESP32).

## 📥 Installation
1. Cloner le dépôt.
2. Installer la bibliothèque **ArduinoJson**.
3. Téléverser le code sur l'ESP32.
4. **Important** : Utiliser l'outil "ESP32 LittleFS Data Upload" pour envoyer le contenu du dossier `/data`.

## 👨‍🏫 Usage Pédagogique
Ce projet sert de support pour étudier :
- La structuration de données en **JSON**.
- Le protocole **HTTP** (requêtes GET/POST).
- La gestion de la mémoire non-volatile sur microcontrôleur.
- Le développement d'interfaces web asynchrones (Fetch API).
