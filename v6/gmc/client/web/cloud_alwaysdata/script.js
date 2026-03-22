/**
	Côté Serveur (Alwaysdata) 

	gérer (température + état du voyant)
	
*/
// Fonction de rafraîchissement des données
function refresh() {
    fetch('index.php') 
        .then(res => {
            if (!res.ok) throw new Error("Erreur serveur");
            return res.json();
        })
        .then(data => {
            // Affichage Température
            document.getElementById('temp').innerText = (data.temp / 10).toFixed(1) + " °C";
            
            // Affichage Date
            document.getElementById('date').innerText = "Reçu le : " + data.server_time;

            // Affichage État (Booléen)
            const etatLabel = document.getElementById('etat_voyant');
            if (data.etat_voyant === true) {
                etatLabel.innerText = "ÉTAT VOYANT : OUI (Alerte)";
                etatLabel.style.color = "red";
            } else {
                etatLabel.innerText = "ÉTAT VOYANT : NON (Normal)";
                etatLabel.style.color = "green";
            }
        })
        .catch(err => {
            console.error("Erreur de lecture :", err);
            document.getElementById('date').innerText = "Erreur de connexion au serveur";
        });
}

// Lancement automatique
setInterval(refresh, 5000); 
refresh();

