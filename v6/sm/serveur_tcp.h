

/**
 * @file serveur_tcp.h
 * @brief Définition de la classe ServeurTcp pour gérer les connexions TCP.
 *
 * Utilisation des options de précompilation:
 * - _WIN32: Défini sur les systèmes Windows.
 * - __linux__: Défini sur les systèmes Linux.
 *
 * Commande de compilation (exemple Linux):
 * g++ -o serveur_secuvia serveur_secuvia_linux.cpp serveur_tcp.cpp client_connecte.cpp fichier_log.cpp predictive_ia.cpp serveur_dao.cpp -lmysqlclient
 *
 * Commande de compilation (exemple Windows, nécessite configuration du connecteur MySQL C++):
 * g++ -o serveur_secuvia serveur_secuvia_linux.cpp serveur_tcp.cpp client_connecte.cpp fichier_log.cpp predictive_ia.cpp serveur_dao.cpp -I"C:\path\to\mysql\connector\include" -L"C:\path\to\mysql\connector\lib" -lmysqlcppconn
 *
 * Commande d'exécution (Linux et Windows):
 * ./serveur_secuvia
 */

#ifndef SERVEUR_TCP_H
#define SERVEUR_TCP_H

#include <string>
#include <cstring>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#endif


class ServeurTcp {
private:
#ifdef _WIN32
    SOCKET serverSocket;
#elif __linux__
    int serverSocket;
#endif
    bool isListening;

public:
    ServeurTcp();
    ~ServeurTcp();
    bool start (std::string ip, int port);
    bool stop();

private:
    std::string ip;
    int port;

#ifdef _WIN32
    SOCKET acceptClient();
#elif __linux__
    int acceptClient();
#endif
    void handleClientConnection(int clientSocket);
    // Vous pourriez utiliser une std::function pour rendre le traitement du client configurable
    std::function<void(int)> clientHandler;
};

#endif // SERVEUR_TCP_H

