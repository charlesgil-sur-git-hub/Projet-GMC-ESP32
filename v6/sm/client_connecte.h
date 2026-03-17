/**
 * @file client_connecte.h
 * @brief Définition de la classe ClientConnecte pour gérer la connexion d'un client.
 *
 * Utilisation des options de précompilation:
 * - _WIN32: Défini sur les systèmes Windows.
 * - __linux__: Défini sur les systèmes Linux.
 */

#ifndef CLIENT_CONNECTE_H
#define CLIENT_CONNECTE_H

#include <string>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <winsock2.h>
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class FichierLog;
class PredectiveIA;
class ServeurDAO;

class ClientConnecte {
private:
    int clientSocket;
    std::string ipAddress;
    std::string connectionDateTime;
    int clientType;

public:
    ClientConnecte(int socket);
    ~ClientConnecte();

    void handleConnection();

private:
    int getClientType();
    bool traitementClientBadge();
    bool traitementClientMobile();

    std::string recvLine();
    std::string recvLineWithTimeout(int sock, int timeout_sec = 5, int max_attempts = 3);
    
    std::string trim(const std::string& s);
    void traceRecv (const std::string& txt, int nb, const std::string& buf);
    void traceSend (const std::string& txt);
    int bytesReadRecvLine;
};

#endif // CLIENT_CONNECTE_H


