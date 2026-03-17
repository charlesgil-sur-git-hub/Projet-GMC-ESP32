

const ESP32_IP = "192.168.1.16"; // <--- METS TON IP ICI

async function fetchHistory() {
    const select = document.getElementById('mesures-select');
    const status = document.getElementById('status');
    
    status.innerText = "Connexion à l'ESP32...";
    
    try {
        const response = await fetch(`http://${ESP32_IP}/api/history_client`);
        const donnees = await response.json();
        
        // On vide la liste actuelle
        select.innerHTML = "";
        
        if (donnees.length === 0) {
            status.innerText = "Historique vide sur l'ESP32.";
            return;
        }

        // On remplit la liste déroulante
        donnees.forEach(m => {
            const option = document.createElement('option');
            const tempC = (m.temp / 10).toFixed(1);
            option.text = `📅 ${m.date} --> 🌡️ ${tempC}°C`;
            select.add(option);
        });
        
        status.innerText = `Succès : ${donnees.length} mesures récupérées.`;

    } catch (error) {
        status.innerText = "Erreur : Impossible de joindre l'ESP32.";
        console.error("Erreur de collecte:", error);
    }
}