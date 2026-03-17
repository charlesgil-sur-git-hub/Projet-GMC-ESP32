/**
 * @file client_connecte.cpp
 * @brief Code de la classe ClientConnecte pour gérer la connexion d'un client.
 *
 * Utilisation des options de précompilation:
 * - _WIN32: Défini sur les systèmes Windows.
 * - __linux__: Défini sur les systèmes Linux.
 * 
 * 
 *  https://tala-informatique.fr/index.php?title=C_socket
 */



#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "client_connecte.h"

#ifdef _LOGGER_ACTIF
    #include "fichier_log.h"
#endif
#ifdef IA_ACTIF
    #include "predictive_ia.h"
#endif
#ifdef DAO_ACTIF
    #include "serveur_dao.h"
#endif




#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __linux__
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#endif

using namespace std;

// #define debug


ClientConnecte::ClientConnecte(int socket) : clientSocket(socket), clientType(0) {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    getpeername(clientSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    ipAddress = inet_ntoa(clientAddr.sin_addr);

    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    connectionDateTime = ss.str();

    std::cout << "\nClient connecte depuis [" << ipAddress << "] en date de " << connectionDateTime << std::endl;
    clientType = getClientType();
    std::cout << "Type de client detecte : " << clientType << std::endl;

}

ClientConnecte::~ClientConnecte() {}

void ClientConnecte::handleConnection() {
    bool bRet = 0;
    
   
    if (clientType == 1) {
        bRet = traitementClientBadge();
    } else if (clientType == 2) {
        bRet = traitementClientMobile();
    } else {
        std::cerr << "Type de client inconnu pour " << ipAddress << std::endl;
        std::string response = "ERROR: Type de client inconnu\n";
        send(clientSocket, response.c_str(), response.length(), 0);
    }
}

int ClientConnecte::getClientType() {
    char buffer[32];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_PEEK); // Peek pour ne pas consommer la requête
    if (bytesRead > 0) {
        std::string peekedData(buffer);
        if (peekedData.find("SEC_MSG_START") == 0) {
            return 1;
        } else if (peekedData.find("SEC_MOB_START") == 0) {
            std::string msgRecu = "SEC_MOB_START";
            cout << "message recu ["<<trim(msgRecu) <<"]" << endl;
            return 2;
        }
    }
    return 0;
}

bool ClientConnecte::traitementClientBadge() {

#ifdef _LOGGER_ACTIF
    FichierLog logger;
#endif
#ifdef _DAO_ACTIF
    ServeurDAO dao;
#endif

    cout << "traitementClientBadge() debut..." << endl;

    #ifdef _DAO_ACTIF
        if (!dao.connecter()) {
            std::cerr << "Erreur de connexion à la base de donnees pour le client badge " << ipAddress << std::endl;
            std::string response = "ERROR: Connexion BD\n";
            send(clientSocket, response.c_str(), response.length(), 0);
            return false;
        }
    #endif

    // Étape 1A : Réception du message SEC_MSG_START

    std::string request = recvLine();
    if (request.find("SEC_MSG_START") == 0) {

        traceRecv ("\n\t1A_SEC_MSG_START attendu \t - Recu ", bytesReadRecvLine, request);


        // Étape 1B : Emission du message SEC_MSG_START_OK

        std::string response = "SEC_MSG_START_OK\n";
        send(clientSocket, response.c_str(), response.length(), 0);
        traceSend ("\t1B_SEC_MSG_START_OK envoye");

        // Étape 2A : Réception du message de données (SEC_MSG_DATE_...)

        std::string dataMessage = recvLine();
        if (dataMessage.find("SEC_MSG_DATE_") == 0) {

            traceRecv ("\n\t2A_SEC_MSG_DATE_ attendu \t - Recu", bytesReadRecvLine, dataMessage);


            std::istringstream iss(dataMessage);
            std::string segment;




            // Étape 2B : Emission du message SEC_MSG_OK

            response = "SEC_MSG_OK\n";
            send(clientSocket, response.c_str(), response.length(), 0);
            traceSend ("\t2B_SEC_MSG_OK envoye");


            // Étape 3A : Réception de SEC_MSG_STOP

            std::string endMessage = recvLine();
            if (trim(endMessage) == "SEC_MSG_STOP") {
                traceRecv ("\n\t3A_SEC_MSG_STOP attendu \t - Recu", bytesReadRecvLine, endMessage);


                // Étape 3B : Emission du message SEC_MSG_STOP_OK

                response = "SEC_MSG_STOP_OK\n";
                send(clientSocket, response.c_str(), response.length(), 0);
                traceSend ("\t3B_SEC_MSG_STOP_OK envoye");

            } else {
                response = "ERROR: Fin de message incorrect\n";
                send(clientSocket, response.c_str(), response.length(), 0);
            }


        } else {
            std::string response = "ERROR: Message de donnees attendu\n";
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    } else {
        std::string response = "ERROR: Demarrage de message incorrect\n";
        send(clientSocket, response.c_str(), response.length(), 0);
    }

    #ifdef _DAO_ACTIF
        dao.deconnecter();
    #endif

    cout << "traitementClientBadge() fin" << endl;

    return true;
}




/**
 *  Traitement pour discussion avec application (dev sur AppInventor)
 */
#ifndef TRAITEMENT_MOBILE
bool ClientConnecte::traitementClientMobile() {return 0;}
#else
bool ClientConnecte::traitementClientMobile() {
    FichierLog logger;
    PredectiveIA ia;
    
    std::string msgAttendu="";
    std::string msgRecu="";
    std::string msgEnvoye="";
    int nbCars = 0;

     cout << "traitementClientMobile() debut..." << endl;

    // Étape 1A : Réception du message SEC_MOB_START

    msgRecu = recvLine();
    if (trim(msgRecu) == "SEC_MOB_START") {
    
        traceRecv ("\t1A_SEC_MOB_START \t <== Recu ", bytesReadRecvLine, msgRecu);
    
    
    
        // Étape 1B : Emission du message SEC_MSG_START_OK
        msgEnvoye = "SEC_MOB_START_OK\n";
        //cout << "\tenvoi message ["<<trim(msgEnvoye) <<"]" << endl;
        nbCars = (int) send(clientSocket, msgEnvoye.c_str(), msgEnvoye.length(), 0);
        if (nbCars==msgEnvoye.length()) {
            
            traceSend ("\t1B SEC_MOB_START_OK \t ==>");
            
            
            // 2A : attente reception SEC_LECTURE_IA

            msgAttendu = "SEC_LECTURE_IA";
            //cout << "\tattente message ["<<trim(msgAttendu) <<"]..." << endl;

            
            //msgRecu = recvLineWithTimeout(clientSocket);
            msgRecu = recvLine();
        
            if (trim(msgRecu) == "SEC_LECTURE_IA") {

                traceRecv ("\t2A SEC_LECTURE_IA \t <== Recu ", bytesReadRecvLine, msgRecu);

                msgRecu = ""; // on vide
                
                /*auto now = std::chrono::system_clock::now();
                auto now_c = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss_date;
                ss_date << std::put_time(std::localtime(&now_c), "%Y-%m-%d");
                std::string today = ss_date.str();
                */
                std::string today = "2025-05-05";

                //cout << "\tanalyseIA'..." << endl;
                std::string iaResult = ia.analyseIA(today); // Analyser tous les événements du jour
                
                
                //cout << "\tanalyseIA fin" << endl;

                std::string responseIA;
                if (!iaResult.empty()) {
                    responseIA = "SEC_MOB_IA_DETECT_YES";
                    // Ici, vous devez récupérer les données pertinentes à envoyer à Appinventor
                    // Ces données pourraient être extraites de l'analyse IA ou d'autres sources
                    // Pour l'exemple, on envoie juste le résultat de l'analyse
                    responseIA += "_#" + iaResult + "\n";
                } else {
                    responseIA = "SEC_MOB_IA_DETECT_NO\n";
                }
            
                // Étape 2B : Emission du message SEC_MOB_IA
                msgEnvoye = responseIA;
                //cout << "\tenvoi message ["<<trim(msgEnvoye) <<"]" << endl;
                nbCars = (int) send(clientSocket, msgEnvoye.c_str(), msgEnvoye.length(), 0);
                if (nbCars==msgEnvoye.length()) {
                    
                    traceSend ("\t2B SEC_MOB_IA_DETECT \t ==>");

                
                    


                } //  if (nbCars==msgEnvoye.length()) {
                
            } //         if (trim(msgRecu) == "LECTURE_IA") {


             // Étape 3A : Réception de SEC_MOB_STOP
             std::string endMessage ="";
            if (msgRecu.length()<2) {
                // si on a videe
                endMessage = recvLine();
            } else {
                endMessage=msgRecu;
            }

            if (trim(endMessage) == "SEC_MOB_STOP") {
                        traceRecv ("\t3A SEC_MOB_STOP \t <== Recu ", bytesReadRecvLine, endMessage);


                        // Étape 3B : Emission du message SEC_MSG_STOP_OK

                        msgEnvoye = "SEC_MOB_STOP_OK\n";
                        send(clientSocket, msgEnvoye.c_str(), msgEnvoye.length(), 0);
                        traceSend ("\t3B SEC_MOB_STOP_OK \t ==>");

                        cout << "traitementClientMobile() fin OK" << endl << endl;
                        return true;

                    } else {
                        msgEnvoye = "ERROR: Fin de message incorrect\n";
                        send(clientSocket, msgEnvoye.c_str(), msgEnvoye.length(), 0);
                    }
            
        } // if (nbCars==msgEnvoye.length()) { traceSend ("\tSEC_MOB_START_OK a bien ete envoye");
    
        
    } else { // if (trim(msgRecu) == "SEC_MOB_START") {
            std::string response = "ERROR: Commande mobile non attendue\n";
            send(clientSocket, response.c_str(), response.length(), 0);
            
    }

    cout << "traitementClientMobile() fin en erreur" << endl << endl;
    return false;

} // traitementClientMobile()

#endif // #ifdef TRAITEMENT_MOBILE


/*
 * 
 */

 std::string ClientConnecte::recvLine() {
    std::string line="";
    char buffer[1];
#ifdef _WIN32
    int bytesRead;
#elif __linux__
    ssize_t bytesRead;
#endif
    bytesReadRecvLine=0;
    
    //cout << "\t\trecvLine...";
#ifdef _WIN32
#elif __linux__
    usleep(10000);
#endif

    while ((bytesRead = recv(clientSocket, buffer, 1, 0)) > 0) {
#ifdef _WIN32
#elif __linux__
    usleep(10000);
#endif

        
        if (buffer[0] == '\n') {
            break;
        }
        line += buffer[0];
        bytesReadRecvLine++;
    }
#ifdef __linux__
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
#elif _WIN32
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
#endif

     


    
    //cout << "\t\trecvLine Fin : line [" << line << "]" <<endl;

    return line;
}


std::string ClientConnecte::recvLineWithTimeout(int sock, int timeout_sec, int max_attempts) {
    std::string line="\n";
    char buffer[1];
#ifdef _WIN32
    int bytesRead;
#elif __linux__
    ssize_t bytesRead;
#endif

    int attempts = 0;
    
    cout << "\trecvLineWithTimeout...";

    while (attempts < max_attempts) {
        attempts++;
        line = "\n";
        


        // Configuration du délai d'attente pour select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        struct timeval timeout;
        timeout.tv_sec = timeout_sec;
        timeout.tv_usec = 0;

        // Utilisation de select pour attendre des données avec un délai
        int retval = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

        if (retval > 0) {
            // Des données sont disponibles, on tente de lire un caractère
            /*
            
            bytesRead = recv(sock, buffer, 1, 0);
            if (bytesRead > 0) {
                if (buffer[0] == '\n') {
                    break; // Fin de ligne
                }
                line += buffer[0];
            } else if (bytesRead == 0) {
                // Connexion fermée par le client
                std::cerr << "Client déconnecté pendant la lecture (recv)." << std::endl;
                return ""; // Retourner une chaîne vide pour indiquer une fin de connexion
            } else {
                // Erreur lors de la lecture
                perror("recv failed in recvLineWithTimeout");
                return ""; // Retourner une chaîne vide en cas d'erreur
            }
            */
            
            while ((bytesRead = recv(clientSocket, buffer, 1, 0)) > 0) {

                
                if (bytesRead > 0) {
                    if (buffer[0] == '\n') {
                        break;
                    }
                    line += buffer[0];
                } else if (bytesRead == 0) {
                // Connexion fermée par le client
                    std::cerr << "Client déconnecté pendant la lecture (recv)." << std::endl;
                    return ""; // Retourner une chaîne vide pour indiquer une fin de connexion
                } else {
                    // Erreur lors de la lecture
                    perror("recv failed in recvLineWithTimeout");
                    return ""; // Retourner une chaîne vide en cas d'erreur
                }
            }
            
        } else if (retval == 0) {
            // Délai d'attente expiré, on continue les tentatives
            std::cerr << "Délai d'attente expiré (tentative " << attempts << ")." << std::endl;
        } else {
            // Erreur lors de l'appel à select
            perror("select failed in recvLineWithTimeout");
            return ""; // Retourner une chaîne vide en cas d'erreur
        }
    }

    // Suppression du \r si présent (gestion des fins de ligne Windows)
#ifdef __linux__
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
#elif _WIN32
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
#endif


     
    
    //cout << "\n\trecvLineWithTimeout Fin : line [" << trim(line) << "]" <<endl;


    return trim(line);
}


#include <algorithm>
#include <cctype>

std::string ClientConnecte::trim(const std::string& s) {
    auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c){ return std::isspace(c); });
    auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c){ return std::isspace(c); }).base();
    return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));

}


/**
 * Fct de trace de reception
 *
 */
void ClientConnecte::traceRecv (const std::string& txt, int nb, const std::string& buf) {
    cout << txt ;
    if (nb>0) {
        cout << "("<< nb << " octets) \t: [";
        for (char c : buf) {
            cout  << c;
        }

        #ifdef debug
        cout << " (";
        for (char c : buf) {
            cout << static_cast<int>(static_cast<unsigned char>(c)) << " ";
        }
        cout << ") ";
        #endif
        cout << "]";
    }
    cout << endl;
}

 /**
 * Fct de trace d emission
 *
 */
void ClientConnecte::traceSend (const std::string& txt) {
    cout << txt ;
    cout << endl;
 }



