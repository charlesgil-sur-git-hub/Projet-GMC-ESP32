let timeLeft = 15;

function updateTimer() {
    timeLeft--;
    if (timeLeft <= 0) {
        timeLeft = 15; // On repart à 15 après le refresh
        refreshData();
    }
    document.getElementById('timerNext').innerText = `MàJ dans : ${timeLeft}s`;
}

// 1. Détection de l'environnement (définie une seule fois pour tout le script)
const isLocal = 
    window.location.protocol === 'file:' || 
    window.location.hostname === 'localhost' || 
    window.location.hostname === '127.0.0.1';

// 2. Fonction pour mettre à jour l'affichage HTML (centralisée)
function displayData(data) {
	// On convertit 187 en 18.7
    let tempReelle = data.temp / 10;
	// On force l'affichage d'un chiffre après la virgule et on remplace le point par une virgule
    let tempTexte = tempReelle.toFixed(1).replace('.', ',');
	
    document.getElementById('tempValue').innerText = tempTexte;
	document.getElementById('dateValue').innerText = data.date;
    
    // On affiche l'uptime s'il existe (mode réel), sinon un message de test
    const statusText = data.uptime ? `Mise à jour : ${data.uptime}s` : "Simulation réussie (JSON local)";
    document.getElementById('responseField').innerText = statusText;
}

// 3. Fonction principale pour récupérer les données
function refreshData() {
    const badge = document.getElementById('envBadge');

    if (isLocal) {
        // ... ton code de simulation actuel ...
    } else {
        fetch('/api/status')
            .then(response => {
                if (!response.ok) throw new Error();
                return response.json();
            })
            .then(data => {
                // SUCCESS : On est connecté
                displayData(data);
                badge.innerText = "ESP32 Connecté";
                badge.style.backgroundColor = "#4CAF50"; // Vert
            })
            .catch(err => {
                // ERREUR : Le WiFi est coupé ou l'ESP32 redémarre
                document.getElementById('responseField').innerText = "⚠️ Connexion perdue...";
                badge.innerText = "🔴 Déconnecté - Tentative...";
                badge.style.backgroundColor = "#f44336"; // Rouge
                
                // Optionnel : on remet les valeurs à "--" pour ne pas tromper l'utilisateur
                document.getElementById('tempValue').innerText = "--";
            });
    }
}



// 4. Initialisation au chargement de la page
document.addEventListener('DOMContentLoaded', () => {
    const badge = document.getElementById('envBadge');

    // 1. Configuration du badge visuel
    if (isLocal) {
        badge.innerText = "Mode Test : Local (PC/Ubuntu)";
        badge.className = "badge mode-local";
    } else {
        badge.innerText = "Mode Réel : ESP32 Connecté";
        badge.className = "badge mode-esp";
    }

    // 2. Un seul écouteur pour le bouton (mise à jour manuelle)
    document.getElementById('actionBtn').addEventListener('click', refreshData);

    // 3. --- AJOUT : MISE À JOUR AUTOMATIQUE ---
    refreshData();              // On l'appelle une fois au démarrage
    //setInterval(refreshData, 15000); // Puis toutes les 15 secondes (5000 ms)
	// Dans ton DOMContentLoaded, remplace le setInterval(refreshData, 15000) par :
	setInterval(updateTimer, 1000);
});

window.onload = function() {
    // Dès que la page charge, on envoie l'heure du PC/Smartphone à l'ESP32
    let now = Math.floor(Date.now() / 1000); // Temps Unix en secondes
    fetch(`/api/sync_time?t=${now}`)
        .then(response => console.log("Horloge synchronisée avec le navigateur !"));
};


// du DOMContentLoaded
document.getElementById('actionBtn').addEventListener('click', () => {
    fetch('/api/led')
        .then(response => response.json())
        .then(data => {
            // On affiche le résultat dans ton champ responseField
            const field = document.getElementById('responseField');
            field.innerText = "La LED est maintenant : " + data.status;
            
            // Optionnel : changer la couleur du bouton selon l'état
            const btn = document.getElementById('actionBtn');
            if(data.status === "ON") {
                btn.style.backgroundColor = "#4CAF50"; // Vert
            } else {
                btn.style.backgroundColor = "#f44336"; // Rouge
            }
        })
        .catch(err => console.error("Erreur LED :", err));
});

