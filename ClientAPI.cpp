#include "ClientAPI.h"      // My H file
#include <sys/types.h>      // socket, bind
#include <sys/socket.h>     // socket, bind, listen, inet_ntoa
#include <netinet/in.h>     // htonl, htons, inet_ntoa
#include <arpa/inet.h>      // inet_ntoa
#include <netdb.h>          // gethostbyname
#include <unistd.h>         // read, write, close
#include <strings.h>        // bzero
#include <netinet/tcp.h>    // SO_REUSEADDR
#include <cstring>          // strlen
//#include <iostream>         // cout (debugging)

/*===============================================================
||                      Private Constants                      ||
===============================================================*/
const int BROADCAST_PORT = 2927;    // The server discovery port
const int GAME_PORT = 2928;         // The server game port

/*===============================================================
||                       Public Functions                      ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          StartClient                          *
*------------------------- Description -------------------------*
* Sets up a UDP socket on the passed in broadcastSocket used for*
* finding a server.                                             *
*                                                               *
*------------------------- Parameters --------------------------*
* int& broadcastSocket: an int that will represent the UDP      *
*   socket after the function completes.                        *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void StartClient(int& broadcastSocket)
{
    // --- set up udp socket ---
    // Start the client
    broadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);

    int broadcastEnable = 1;
    setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    sockaddr_in broadcastSock;
    bzero((char*) &broadcastSock, sizeof(broadcastSock));
    broadcastSock.sin_family = AF_INET;
    broadcastSock.sin_port = htons(BROADCAST_PORT);
    broadcastSock.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // look for servers
    const char* message = "temp";   // Decide what to put in message (maybe player name?)
    sendto(broadcastSocket, message, sizeof(message), 0, (struct sockaddr*)&broadcastSock, sizeof(broadcastSock));
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          JoinServer                           *
*------------------------- Description -------------------------*
* Sets up a TCP socket on the gameSocket using the              *
* broadcastSocket to find the server and request it's IP.       *
*                                                               *
*------------------------- Parameters --------------------------*
* const int broadcastSocket: A UDP socket set up with           *
*   StartClient().                                              *
*                                                               *
* int& gameSocket: an int that will represent the TCP socket    *
*   after the function completes.                               *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void JoinServer(const int broadcastSocket, int& gameSocket)
{
    // --- Find IP of server over UDP ---
    bool foundServer = false;

    char hostName[256];
    struct hostent* host;
    sockaddr_in serverResponseSock;
    socklen_t serverResponseLen = sizeof(serverResponseSock);
    
    while (!foundServer)
    {
        ssize_t bytesRead = recvfrom(broadcastSocket, hostName, sizeof(hostName), 0, (struct sockaddr*)&serverResponseSock, &serverResponseLen);
        if (bytesRead > 0)
        {
            host = gethostbyname(hostName);
            foundServer = true;
        }
    }

    // --- Establish a TCP connection with  the server ---
    // Set up the data structure
    sockaddr_in gameSock;
    bzero((char*) &gameSock, sizeof(gameSock));
    gameSock.sin_family = AF_INET;
    gameSock.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    gameSock.sin_port = htons(GAME_PORT);

    gameSocket = socket(AF_INET, SOCK_STREAM, 0);
    // Connect <-- this makes me a client!
    connect(gameSocket, (sockaddr*)&gameSock, sizeof(gameSock));
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                       ReadDataFromServer                      *
*------------------------- Description -------------------------*
* Reads bufferSize bytes from the socket into buffer using TCP  *
*                                                               *
*------------------------- Parameters --------------------------*
* const int socket: A TCP socket used to comunicate with the    *
*   server about the game. Set up using JoinServer().           *
*                                                               *
* char buffer[]: Where the data read from the server will be    *
*   placed.                                                     *
*                                                               *
* const int bufferSize: The amount of bytes to read from the    *
*   server into the buffer.                                     *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ReadDataFromServer(const int socket, char buffer[], const int bufferSize)
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
*                        SendDataToServer                       *
*------------------------- Description -------------------------*
* Sends data to the socket using a TCP connection.              *
*                                                               *
*------------------------- Parameters --------------------------*
* const int socket: A TCP socket used to comunicate with the    *
*   server about the game. Set up using JoinServer().           *
*                                                               *
* const char data[]: The data to send to the server.            *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool SendDataToServer(const int socket, const char data[])
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
* const int socket: A socket used to comunicate with the server.*
*   Can be UDP or TCP.                                          *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void CloseConnection(const int socket)
{
    close(socket);
}