#include "ClientAPI.h"
#include "ClientConnection.h"
#include <string>
#include <stdio.h>
#include <iostream>
#include <cstring>

/*===============================================================
||                       Public Functions                      ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          Constructor                          *
*------------------------- Description -------------------------*
* Initalizes the private variables.                             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
ClientConnection::ClientConnection()
{
    tcpConnection = -1;
    isRegistered = false;
    isInGame = false;
    prevMoney = -1;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          Destructor                           *
*------------------------- Description -------------------------*
* Exit and Unregisters the client if needed and closes the      *
* socket.                                                       *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
ClientConnection::~ClientConnection()
{
    Unregister();
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           Register                            *
*------------------------- Description -------------------------*
* Establish a TCP connection with a server.                     *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ClientConnection::Register()
{
    int udpSocket;
    StartClient(udpSocket);
    JoinServer(udpSocket, tcpConnection);
    CloseConnection(udpSocket);
    isRegistered = true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           ListGames                           *
*------------------------- Description -------------------------*
* Request the server to send a list of available games and      *
* recieve it's response.                                        *
*                                                               *
*------------------------- Parameters --------------------------*
* char* buffer: Where the data read from the server will be     *
* placed.                                                       *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::ListGames(char* buffer)
{
    // auto fail if not connected to server
    // auto fail if in a game
    if ((!isRegistered) || isInGame)
    {
        return true;
    }

    bool hasSucceded = true;
    // Ask server for list of games
    hasSucceded = SendDataToServer(tcpConnection, LIST_GAME_REQUEST);
    if (!hasSucceded)
    {
        return false;
    }

    // recive list of games from server
    hasSucceded = ReadDataFromServer(tcpConnection, buffer, LIST_GAME_BUFFER_SIZE);
    if (!hasSucceded)
    {
        return false;
    }

    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          CreateGame                           *
*------------------------- Description -------------------------*
* Request the server to create a game with the given name. If   *
* the game name is valid, then it will call JoinGame() with the *
* provided game name.                                           *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string name: The name of the game to create.       *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::CreateGame(const std::string name)
{
    // auto fail if not connected to server
    // auto fail if already in a game
    if ((!isRegistered) || isInGame)
    {
        return true;
    }

    // ensure room name is smaller than the max name length

    bool hasSucceded = true;
    // Ask server for to make a game with a name
    hasSucceded = SendDataToServer(tcpConnection, CREATE_REQUEST);
    hasSucceded = hasSucceded && SendDataToServer(tcpConnection, name.c_str());
    if (!hasSucceded)
    {
        return false;
    }
    // recive response from server
    char response[sizeof(SERVER_TRUE) + 1];

    hasSucceded = ReadDataFromServer(tcpConnection, response, sizeof(SERVER_TRUE) + 1);
    if (!hasSucceded)
    {

        return false;
    }

    // handle server response
    if (std::string(response).compare(std::string(SERVER_TRUE)) == 0)
    {
        return JoinGame(name);
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           JoinGame                            *
*------------------------- Description -------------------------*
* Request the server to join a game with the given name.       *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string name: The name of the game to join.         *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::JoinGame(const std::string name)
{
    // auto fail if not connected to server
    // auto fail if already in a game
    if ((!isRegistered) || isInGame)
    {
        return true;
    }

    bool hasSucceded = true;
    // Ask server for to join a game with a name
    hasSucceded = SendDataToServer(tcpConnection, JOIN_REQUEST);
    hasSucceded = hasSucceded && SendDataToServer(tcpConnection, name.c_str());
    if (!hasSucceded)
    {
        return false;
    }

    // recive response from server
    char response[sizeof(SERVER_TRUE) + 1];
    memset(response, 0, sizeof(response));
    hasSucceded = ReadDataFromServer(tcpConnection, response, sizeof(SERVER_TRUE) + 1);
    if (!hasSucceded)
    {
        return false;
    }

    // handle server response
    if (std::string(response).compare(std::string(SERVER_TRUE)) == 0)
    {
        isInGame = true;
    }
    return true;


}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           ExitGame                            *
*------------------------- Description -------------------------*
* Inform the server that you are disconnecting from the game.   *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ClientConnection::ExitGame()
{
    // Infrom the server you are leaving the game
    SendDataToServer(tcpConnection, EXIT_REQUEST);
    // Clean up local state 
    CloseConnection(tcpConnection);
    tcpConnection = -1;
    isInGame = false;
    isRegistered = false;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          Unregister                           *
*------------------------- Description -------------------------*
* Inform the server that you are disconnecting from the lobby.  *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ClientConnection::Unregister()
{
    // Infrom the server you are leaving the lobby
    SendDataToServer(tcpConnection, EXIT_REQUEST);
    // Clean up local state 
    CloseConnection(tcpConnection);
    tcpConnection = -1;
    isRegistered = false;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                              Bet                              *
*------------------------- Description -------------------------*
* Tell the server the amount to bet.                            *
*                                                               *
*------------------------- Parameters --------------------------*
* const int val: The amount to bet.                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::Bet(const int val)
{

    // auto fail if not connected to server
    // auto fail if not in a game
    if ((!isRegistered) || (!isInGame))
    {
        return true;
    }

    bool hasSucceded = true;
    // Ask server to bet an amount
    char str[BET_BUFFER_SIZE];
    sprintf(str,"%d", val);
    hasSucceded = SendDataToServer(tcpConnection, BET_REQUEST);
    hasSucceded = hasSucceded && SendDataToServer(tcpConnection, str);
    if (!hasSucceded)
    {
        return false;
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                              Hit                              *
*------------------------- Description -------------------------*
* Tell the server to hit.                                       *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::Hit()
{

    // auto fail if not connected to server
    // auto fail if not in a game
    if ((!isRegistered) || (!isInGame))
    {
        return true;
    }

    bool hasSucceded = true;
    // Ask server to hit
    hasSucceded = SendDataToServer(tcpConnection, HIT_REQUEST);
    if (!hasSucceded)
    {
        return false;
    }

    return true;
    
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                             Stand                             *
*------------------------- Description -------------------------*
* Tell the server to stand.                                     *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::Stand()
{

    // auto fail if not connected to server
    // auto fail if not in a game
    if ((!isRegistered) || (!isInGame))
    {
        return true;
    }

    bool hasSucceded = true;
    // Ask server to stand
    hasSucceded = SendDataToServer(tcpConnection, STAND_REQUEST);
    if (!hasSucceded)
    {
        return false;
    }

    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          GetStateData                         *
*------------------------- Description -------------------------*
* Get a state structure from the server that described the      *
* current state of the game. Pass by refrence due to struct     *
* using a vector for each player's and the dealer's hands.      *
*                                                               *
*------------------------- Parameters --------------------------*
* StateData &state: The state to have the data from the server  *
*   replace.                                                    *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ClientConnection::GetStateData(StateData &state)
{
    // read in the data from the server
    bool stillConnected;
    char MultiCharBuffer[FRAME_BUFFER_SIZE];
    stillConnected = ReadDataFromServer(tcpConnection, MultiCharBuffer, FRAME_BUFFER_SIZE);
    if (!stillConnected)
    {
        return false;
    }
    std::string data(MultiCharBuffer);
    char endOfItem = '_';
    int start = 0;
    int end;

    // send conformation of data aquisition;
    stillConnected = SendDataToServer(tcpConnection, SERVER_TRUE);
    if (!stillConnected)
    {
        return false;
    }

    // set playerIndex
    end = data.find(endOfItem, start);
    state.playerIndex = atoi(data.substr(start, end-start).c_str());
    start = end + 1;
    
    // set playerIndex
    end = data.find(endOfItem, start);
    state.playerTurn = atoi(data.substr(start, end-start).c_str());
    start = end + 1;

    // set newRound
    end = data.find(endOfItem, start);
    int temp= atoi(data.substr(start, end-start).c_str());
    start = end + 1;
    if(temp == 1)
    {
        state.isNewRound = true;
    }
    else
    {
        state.isNewRound = false;
    }

    //set player money
    for(int i = 0; i <= PLAYER_COUNT; i++)
    {
        end = data.find(endOfItem, start);
        state.playerMoney[i] = atoi(data.substr(start, end-start).c_str());
        start = end + 1;

        // set the player last money
        if (i == state.playerIndex)
        {
            prevMoney = state.playerMoney[i];
        }
    }

    //set player bets
    for(int i = 0; i <= PLAYER_COUNT; i++)
    {
        end = data.find(endOfItem, start);
        state.playerBets[i] = atoi(data.substr(start, end-start).c_str());
        start = end + 1;
    }

    //set player hands
    for(int i = 0; i <= PLAYER_COUNT; i++)
    {
        end = data.find(endOfItem, start);
        temp = atoi(data.substr(start, end-start).c_str());
        start = end + 1;
        state.shownCards[i].clear();
        // send each card
        for(int j = 0; j < temp; j++)
        {
            end = data.find(endOfItem, start);
            state.shownCards[i].push_back(data.substr(start, end-start).c_str());
            start = end + 1;
        }
    }

    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          GetPrevMoney                         *
*------------------------- Description -------------------------*
* Get the last amount of money the client had.                  *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns the amount of money the player had last.              *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
int ClientConnection::GetPrevMoney()
{
    return prevMoney;
}