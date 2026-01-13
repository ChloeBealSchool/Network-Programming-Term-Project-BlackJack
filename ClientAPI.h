#ifndef CLIENTAPI_H
#define CLIENTAPI_H

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
void StartClient(int& broadcastSocket);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          JoinServer                           *
*------------------------- Description -------------------------*
* Sets up a TCP socket on the gameSocket using the              *
* broadcastSocket to find the server and request it's IP used in*
* the TCP socket.                                               *
*                                                               *
*------------------------- Parameters --------------------------*
* const int broadcastSocket: A UDP socket set up with           *
*   StartClient().                                              *
*                                                               *
* int& gameSocket: an int that will represent the TCP socket    *
*   after the function completes.                               *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void JoinServer(const int broadcastSocket, int& gameSocket);

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
bool ReadDataFromServer(const int socket, char buffer[], const int bufferSize);

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
bool SendDataToServer(const int socket, const char data[]);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        CloseConnection                        *
*------------------------- Description -------------------------*
* Closes a socket connection.                                   *
*                                                               *
*------------------------- Parameters --------------------------*
* const int socket: A socket used to comunicate with the server.*
*   Can be UDP or TCP.                                          *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void CloseConnection(const int socket);

#endif