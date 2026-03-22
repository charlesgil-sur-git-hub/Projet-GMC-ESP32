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

// On récupère le contenu brut du POST (le JSON de l'ESP32)
$json_recu = file_get_contents('php://input');

if (!empty($json_recu)) {
    // On ajoute un timestamp pour savoir quand la donnée est arrivée
    $data = json_decode($json_recu, true);
    $data['server_time'] = date('Y-m-d H:i:s');
    
    // On sauvegarde dans un fichier local sur Alwaysdata
    file_put_contents('data.json', json_encode($data));
    
    echo json_encode(["status" => "success", "message" => "Donnée reçue"]);
} else {
    // Si on consulte la page via un navigateur, on affiche juste le contenu actuel
    if (file_exists('data.json')) {
        echo file_get_contents('data.json');
    } else {
        echo json_encode(["status" => "error", "message" => "Aucune donnée"]);
    }
}
?>