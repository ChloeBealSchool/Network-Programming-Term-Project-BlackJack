#include "ServerAPI.h"      // My H file
#include <sys/types.h>      // socket, bind
#include <sys/socket.h>     // socket, bind, listen, inet_ntoa
#include <netinet/in.h>     // htonl, htons, inet_ntoa
#include <arpa/inet.h>      // inet_ntoa
#include <netdb.h>          // gethostbyname
#include <unistd.h>         // read, write, close
#include <strings.h>        // bzero
#include <netinet/tcp.h>    // SO_REUSEADDR
#include <chrono>           // used for timeouts
#include <cstring>          // strlen
//#include <iostream>         // cout (debugging)

/*===============================================================
||                      Private Constants                      ||
===============================================================*/
const int BROADCAST_PORT = 2927;
const int GAME_PORT = 2928;

/*===============================================================
||                       Public Functions                      ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          StartServer                          *
*------------------------- Description -------------------------*
* Sets up a UDP socket on broadcastSocket used for finding      *
* clients. Sets up a TCP socket on gameSocket used for          *
* communicating about the game.                                 *
*                                                               *
*------------------------- Parameters --------------------------*
* int& broadcastSocket: an int that will represent the UDP      *
*   socket after the function completes.                        *
*                                                               *
* int& gameSocket: an int that will represent the TCP socket    *
*   after the function completes.                               *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void StartServer(int& broadcastSocket, int& gameSocket)
{
    // --- broadcast socket ---
    broadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);

    int broadcastEnable = 1;
    setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    sockaddr_in broadcastSock;
    bzero((char*) &broadcastSock, sizeof(broadcastSock));
    broadcastSock.sin_family = AF_INET;
    broadcastSock.sin_addr.s_addr = htonl(INADDR_ANY);
    broadcastSock.sin_port = htons(BROADCAST_PORT);
    bind(broadcastSocket, (sockaddr*) &broadcastSock, sizeof(broadcastSock));

    // --- game socket ---
    // Create socket
    sockaddr_in gameSock;
    bzero((char*) &gameSock, sizeof(gameSock));  // zero out the data structure
    gameSock.sin_family = AF_INET;   // using IP
    gameSock.sin_addr.s_addr = htonl(INADDR_ANY); // listen on any address this computer has
    gameSock.sin_port = htons(GAME_PORT);  // set the port to listen on
    // Open a stream-oriented socket with the Internet address family
    gameSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Set the SO_REUSEADDR option
    const int on = 1;
    setsockopt(gameSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(int));

    // Bind the socket
    bind(gameSocket, (sockaddr*) &gameSock, sizeof(gameSock));  // bind the socket using the parameters we set earlier
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          AcceptClient                         *
*------------------------- Description -------------------------*
* Recives a connection request from a client on broadcastSocket *
* (UDP) and replies with the Server IP. The establishes a TCP   *
* connection with the client using the gameSocket.              *
*                                                               *
*------------------------- Parameters --------------------------*
* const int broadcastSocket: A UDP socket set up with           *
*   StartServer().                                              *
*                                                               *
* const int gameSocket: A UDP socket set up with StartServer(). *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns an int representing the socket used to communicate    *
* with the client.                                              *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
int AcceptClient(const int broadcastSocket, const int gameSocket)
{
    // --- Send IP to client over UDP ---
    bool foundClient = false;
    char clientBuffer[1024];
    sockaddr_in clientSock;
    socklen_t clientSockSize = sizeof(clientSock);
    
    while (!foundClient)
    {
        ssize_t bytesRead = recvfrom(broadcastSocket, clientBuffer, sizeof(clientBuffer), 0, (struct sockaddr*)&clientSock, &clientSockSize);
        if (bytesRead > 0)
        {

            char localHostName[256];
            gethostname(localHostName, sizeof(localHostName));
            sendto(broadcastSocket, localHostName, sizeof(localHostName), 0, (const sockaddr *)&clientSock, clientSockSize);
            foundClient = true;

        }
    }

    // --- Set up TCP communication with client ---
    // Listen on the socket
    int n = 4;
    listen(gameSocket, n);

    // Recive request from client
    sockaddr_in newsock;   // place to store parameters for the new connection
    socklen_t newsockSize = sizeof(newsock);
    int clientSockTCP = -1;
    while (clientSockTCP < 0)
    {
        clientSockTCP = accept(gameSocket, (sockaddr *)&newsock, &newsockSize);  // grabs the new connection and assigns it a temporary socket
    }
    return clientSockTCP;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                       ReadDataFromClient                      *
*------------------------- Description -------------------------*
* Reads bufferSize bytes from the socket into buffer using TCP  *
*                                                               *
*------------------------- Parameters --------------------------*
* const int socket: A TCP socket used to comunicate with the    *
*   client about the game. Set up using AcceptClient().         *
*                                                               *
* char buffer[]: Where the data read from the client will be    *
*   placed.                                                     *
*                                                               *
* const int bufferSize: The amount of bytes to read from the    *
*   client into the buffer.                                     *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ReadDataFromClient(const int socket, char buffer[], const int bufferSize)
{
    ssize_t bytes = read(socket, buffer, bufferSize - 1);
    if (bytes > 0)
    {
        if (bytes < bufferSize)
        {
            buffer[bytes] = '\0';
        }
        else
        {
            buffer[bufferSize - 1] = '\0';
        }
        return true;
    }
    return false;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        SendDataToClient                       *
*------------------------- Description -------------------------*
* Sends data to the socket using a TCP connection.              *
*                                                               *
*------------------------- Parameters --------------------------*
* const int socket: A TCP socket used to comunicate with the    *
*   client about the game. Set up using AcceptClient().         *
*                                                               *
* const char data[]: The data to send to the client.            *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool SendDataToClient(const int socket, const char data[])
{
    int x = write(socket, data, strlen(data) );
    if ( x < 0 )
    {
        return false;
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        CloseConnection                        *
*------------------------- Description -------------------------*
* Closes a socket connection.                                   *
*                                                               *
*------------------------- Parameters --------------------------*
* const int socket: A socket used to comunicate with the client.*
*   Can be UDP or TCP.                                          *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void CloseConnection(const int socket)
{
    close(socket);
}