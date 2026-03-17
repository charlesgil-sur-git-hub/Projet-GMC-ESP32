/**
 * @file serveur_secuvia_tcp.cpp
 * @brief Programme principal pour le serveur Secuvia sous Linux (et potentiellement Windows avec précompilation).
 *
 * Utilisation des options de précompilation:
 * - _WIN32: Défini sur les systèmes Windows.
 * - __linux__: Défini sur les systèmes Linux.
 *
 * Commande de compilation (exemple Linux):
 * g++ -o serveur_secuvia serveur_secuvia_tcp.cpp serveur_tcp.cpp client_connecte.cpp fichier_log.cpp predictive_ia.cpp serveur_dao.cpp  -pthread -lmysqlclient
 *
 * Commande de compilation (exemple Windows, nécessite configuration du connecteur MySQL C++):
 *

 g++ -o serveur_secuvia serveur_secuvia_tcp.cpp serveur_tcp.cpp client_connecte.cpp fichier_log.cpp predictive_ia.cpp serveur_dao.cpp -I"C:\path\to\mysql\connector\include" -L"C:\path\to\mysql\connector\lib" -lmysqlcppconn -lws2_32 -pthread -std=c++14

 !!! sans mysql mais avec serveur_dao vide
 g++ serveur_secuvia_tcp.cpp serveur_tcp.cpp client_connecte.cpp fichier_log.cpp predictive_ia.cpp  serveur_dao.cpp -lws2_32 -pthread        -o serveur_secuvia

 *
 * Commande d'exécution (Linux et Windows):
 * ./serveur_secuvia
 */

#include "serveur_tcp.h"
#include <iostream>
#include <sstream> 		//! Pour la conversion string => int avec la classe istringstream


int main (int argc, char *argv[]) {

    std::string ip = "";
    int port = 0;

    std::cout << "\n\nProgramme serveur secuvia tcp... " << std::endl;

	//! Arguments
	if (argc < 3)
	{
		std::cout << "Arguments manquants : adresse ip et port" << std::endl;
        std::cout << "\tex : serveur_secuvia 192.168.43.179 8080" << std::endl;

        std::cout << "\tex : serveur_secuvia 192.168.43.205 8080" << std::endl;
        std::cout << "\tou : serveur_secuvia 192.168.43.43 8080" << std::endl;
        std::cout << "\n\n";
		return -2;
	}


    ServeurTcp serveur;


    /**
     *     Parametres par les arguments en ligne de commande
     * */

    //! ip
    ip = "127.0.0.1";       // defaut
    ip = "192.168.1.28";    // mon windows home
    ip = "192.168.66.57";   // asus poste lycee
    ip = "192.168.1.44";
    ip = "192.168.43.205" ; // par wifi S7
    ip = "192.168.43.43" ;  // par wifi S7 ou S9
    std::string chaineTempIp = std::string(argv[1]);
    std::istringstream istrIp (chaineTempIp);
    istrIp >> ip;

    //! port
    port = 8080; // valeur du port par defaut
    std::string chaineTempPort = std::string(argv[2]);
    std::istringstream istrPort (chaineTempPort);
    istrPort >> port;


    if (serveur.start(ip, port)) {
        std::cout << "Serveur démarré avec succès. Appuyez sur une touche pour arrêter..." << std::endl;
        std::cin.get(); // Attendre une entrée pour arrêter le serveur
        serveur.stop();
        std::cout << "Serveur arrêté." << std::endl;
    } else {
        std::cerr << "Erreur lors du démarrage du serveur." << std::endl;
        return 1;
    }

    return 0;
}
