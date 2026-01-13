#ifndef SERVERAPI_H
#define SERVERAPI_H

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
void StartServer(int& broadcastSocket, int& gameSocket);

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
int AcceptClient(const int broadcastSocket, const int gameSocket);

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
bool ReadDataFromClient(const int socket, char buffer[], const int bufferSize);

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
bool SendDataToClient(const int, const char[]);

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
void CloseConnection(const int);

#endif