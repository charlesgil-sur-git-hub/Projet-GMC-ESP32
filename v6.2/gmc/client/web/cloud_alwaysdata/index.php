<?php
/**
	Côté Serveur (Alwaysdata) 
	script qui recoit le JSON et 
	peut enregistrer l info
	
	Ce script va recevoir le JSON de l'ESP32 
	et l'écrire dans un fichier data.json
*/

// On autorise tout le monde à lire (CORS)
header("Access-Control-Allow-Origin: *");
header("Content-Type: application/json");

// 1. On récupère le contenu brut du POST (le JSON de l'ESP32)
$json_recu = file_get_contents('php://input');

// Configuration : Fichier de stockage local (simule une base de données)
$jsonFile = 'data.json';


if (!empty($json_recu)) {
    // On ajoute un timestamp pour savoir quand la donnée est arrivée
    $data = json_decode($json_recu, true);
    $data['server_time'] = date('Y-m-d H:i:s');
    
    // Sauvegarde dans un fichier (ou insertion SQL ici plus tard)
    // FILE_APPEND permet de garder l'historique
    // On sauvegarde dans un fichier local sur Alwaysdata
    //file_put_contents('data.json', json_encode($data));
    file_put_contents($jsonFile, json_encode($data) . PHP_EOL, FILE_APPEND);
    
    // Réponse de succès à l'ESP32
    http_response_code(200);
    echo json_encode(["status" => "success", "message" => "Donnée reçue"]);
} else {
    // Si on consulte la page via un navigateur, on affiche juste le contenu actuel
    if (file_exists($jsonFile)) {
        echo file_get_contents($jsonFile);
    } else {
    	// Réponse d'erreur si le JSON est vide ou mal formé
    	http_response_code(400);
        echo json_encode(["status" => "error", "message" => "Aucune donnée valide"]);
    }
} // if (!empty($json_recu)) {
?>


