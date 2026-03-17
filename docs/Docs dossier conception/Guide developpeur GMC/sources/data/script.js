/**
 * 🛠 GUIDE DÉVELOPPEUR JS :
 * - Pour ajouter une donnée : update displayData()
 * - Pour ajouter un bouton : fetch('/api/ma-route').then(...)
 */

let timeLeft = 15;
let currentUptime = 0;

// 1. Détection de l'environnement (PC vs ESP32)
const isLocal = 
    window.location.protocol === 'file:' || 
    window.location.hostname === 'localhost' || 
    window.location.hostname === '127.0.0.1';

// 2. Gestion du Timer (Rafraîchissement auto)
function updateTimer() {
    timeLeft--;
    if (timeLeft <= 0) {
        timeLeft = 15; 
        refreshData();
    }
    const timerElem = document.getElementById('timerNext');
    if(timerElem) timerElem.innerText = `MàJ dans : ${timeLeft}s`;
}

// 3. Mise à jour de l'affichage (Centralisé)
function displayData(data) {
    // Traitement de la température (ex: 187 -> 18,7)
    let tempReelle = data.temp / 10;
    let tempTexte = tempReelle.toFixed(1).replace('.', ',');
	
    document.getElementById('tempValue').innerText = tempTexte;
    document.getElementById('dateValue').innerText = data.date || "--";
    
	currentUptime = data.uptime;
    const statusText = data.uptime ? `Uptime : ${data.uptime}s` : "Simulation locale";
    document.getElementById('responseField').innerText = statusText;
}

// 4. Récupération des données (Fetch / API)
/*function refreshData() {
    const badge = document.getElementById('envBadge');

    if (isLocal) {
        // SIMULATION pour test sur PC
        console.log("Mode Simulation");
        const mockData = { temp: 215, date: "14:30", uptime: 0 };
        displayData(mockData);
        badge.innerText = "Mode Test : Local (PC)";
        badge.style.backgroundColor = "#ff9800"; // Orange
    } else {
        // RÉEL : Appel à l'ESP32
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                displayData(data);
                badge.innerText = "ESP32 Connecté";
                badge.style.backgroundColor = "#4CAF50"; // Vert
            })
            .catch(err => {
                badge.innerText = "🔴 Déconnecté";
                badge.style.backgroundColor = "#f44336"; // Rouge
            });
    }
}
*/

function refreshData() {
    const badge = document.getElementById('envBadge');
    
    if (isLocal) {
        // SIMULATION pour test sur PC
        console.log("Mode Simulation");
        const mockData = { temp: 215, date: "14:30", uptime: 0 };
        displayData(mockData);
        badge.innerText = "Mode Test : Local (PC)";
        badge.style.backgroundColor = "#ff9800"; // Orange
    } else {
        fetch('/api/status')
            .then(response => {
                if (!response.ok) throw new Error("Erreur serveur " + response.status);
                return response.json();
            })
            .then(data => {
                displayData(data);
                badge.innerText = "✅ ESP32 Connecté";
                badge.style.backgroundColor = "#4CAF50"; // Vert
                // ... mise à jour badge vert ...
            })
            .catch(err => {
                // ICI : On affiche l'erreur sur l'IHM au lieu de rester muet
                const badge = document.getElementById('envBadge');
                badge.innerText = "❌ Erreur de liaison";
                badge.style.backgroundColor = "#f44336";
                console.error("Le serveur ne répond pas :", err);
                
                // On peut même écrire l'erreur dans le champ de réponse
                document.getElementById('responseField').innerText = "Erreur : Impossible de joindre l'ESP32.";
            });
    }
}


// 5. Fonctions spécifiques pour les boutons (SIGNAUX)

// Gestion du bouton "Envoyer Uptime"
document.getElementById('uptimeBtn').addEventListener('click', () => {
    // On récupère la valeur affichée ou une variable
    //const currentUptime = timeLeft; // Ou toute autre donnée numérique
	
    fetch(`/api/get_uptime?valeur=${currentUptime}`)
        .then(response => response.json())
        .then(data => {
            document.getElementById('responseField').innerText = 
                "Serveur a reçu l'uptime : " + data.recu;
        })
        .catch(err => console.error("Erreur Flash :", err));
});

/*function triggerGPIO() {
    fetch('/api/piloter_gpio')
        .then(response => response.text())
        .then(txt => {
            document.getElementById('responseField').innerText = "Action : " + txt;
        });
}
*/
function triggerGPIO(statut) {
    // On construit l'URL : /api/semaphore_on ou /api/semaphore_off
    fetch('/api/piloter_gpio/semaphore_' + statut)
        .then(response => response.text())
        .then(txt => {
            document.getElementById('responseField').innerText = "Etat : " + txt;
        })
         .catch(err => console.error("Erreur pilotage :", err));
}


// 6. Initialisation au chargement
document.addEventListener('DOMContentLoaded', () => {
    
    // Lancement des timers
    refreshData();
    setInterval(updateTimer, 1000);
});

// Synchronisation de l'heure au chargement réel
window.onload = function() {
    if(!isLocal) {
        let now = Math.floor(Date.now() / 1000);
        fetch(`/api/sync_time?t=${now}`);
    }
};
