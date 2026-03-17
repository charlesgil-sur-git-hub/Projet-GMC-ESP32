#include "serveur_tcp.h"
#include "client_connecte.h"
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
using namespace std::chrono_literals;

#pragma comment(lib, "Ws2_32.lib")
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

ServeurTcp::ServeurTcp() : serverSocket(-1), isListening(false) {}

ServeurTcp::~ServeurTcp() {
    stop();
}

bool ServeurTcp::start (std::string ip, int port) {
    this->ip = ip;
    this->port = port;
    
    std::cout << "ServeurTcp::start  [" <<ip<<":" << port << "]" << std::endl;


#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return false;
    }
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "socket creation failed with error: " << WSAGetLastError() << std::endl;
#elif __linux__
    if (serverSocket == -1) {
        perror("socket creation failed");
#endif
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;

    /**
     *     Parametres ip et port
     * */
    if (ip.length()<12) {
         serverAddr.sin_addr.s_addr = INADDR_ANY;
    }
    else {
#ifdef _WIN32        
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str()); // mon windows home
#elif __linux__ 
        // Utiliser inet_pton pour convertir l'adresse IP de texte en format binaire
        if (inet_pton(AF_INET, ip.c_str(), &(serverAddr.sin_addr)) <= 0) {
            std::cerr << "Invalid address/address family" << std::endl;
            close(serverSocket);
            return 1;
        }
#endif        
    }
    serverAddr.sin_port = htons(port);
    
   

    std::cout << "bind sur [" <<serverAddr.sin_addr.s_addr<<":" << serverAddr.sin_port << "]" << std::endl;
     
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))
#ifdef _WIN32
        == SOCKET_ERROR
#elif __linux__
        < 0
#endif
    ) {
#ifdef _WIN32
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
#elif __linux__
        perror("bind failed");
        close(serverSocket);
#endif
        return false;
    }

    if (listen(serverSocket, SOMAXCONN)
#ifdef _WIN32
        == SOCKET_ERROR
#elif __linux__
        < 0
#endif
    ) {
#ifdef _WIN32
        std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
#elif __linux__
        perror("listen failed");
        close(serverSocket);
#endif
        return false;
    }

    std::cout << "Serveur TCP ["<< ip << ":" << port << "] en ecoute..." << std::endl;
    isListening = true;

    while (isListening) {
        int clientSocket = acceptClient();
        if (clientSocket
#ifdef _WIN32
            != INVALID_SOCKET
#elif __linux__
            >= 0
#endif
        ) {


          // cg AVEC thread
#ifdef _WIN32
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));


            std::thread clientThread(&ServeurTcp::handleClientConnection, this, clientSocket);
            clientThread.detach(); // Lancer le client dans un thread séparé

            //handleClientConnection (clientSocket);
#elif __linux__
            std::thread clientThread(&ServeurTcp::handleClientConnection, this, clientSocket);
            clientThread.detach(); // Lancer le client dans un thread séparé
#endif
            
            // cg sans thread
            // OK handleClientConnection (clientSocket);
            

#ifdef _WIN32
            //std::this_thread::sleep_for(std::chrono::milliseconds(x));
            //std::this_thread::sleep_for(1000);
#elif __linux__
            long x = 1000; usleep(x); // delai x
#endif
            
        } else {
#ifdef _WIN32
            std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
#elif __linux__
            perror("accept failed");
#endif
            // Peut-être ajouter une pause ou une logique de réessai ici
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

bool ServeurTcp::stop() {
    isListening = false;
#ifdef _WIN32
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        WSACleanup();
        serverSocket = INVALID_SOCKET;
    }
#elif __linux__
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
#endif
    std::cout << "Serveur TCP arrêté." << std::endl;
    return true;
}

#ifdef _WIN32
SOCKET ServeurTcp::acceptClient() {
    return accept(serverSocket, nullptr, nullptr);
}
#elif __linux__
int ServeurTcp::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    return accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
}
#endif

void ServeurTcp::handleClientConnection(int clientSocket) {
    ClientConnecte client(clientSocket);
    client.handleConnection();
#ifdef _WIN32
    closesocket(clientSocket);
#elif __linux__
    close(clientSocket);
#endif
}


