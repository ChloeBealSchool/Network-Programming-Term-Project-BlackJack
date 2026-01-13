#include "ServerAPI.h"
#include "ServerConnection.h"
#include <string>
#include <string.h>
#include <unordered_map>
#include <iostream>
#include <stdio.h>
#include <cstring>

/*===============================================================
||                      Private Functions                      ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                       NextOpenSeatInRoom                      *
*------------------------- Description -------------------------*
* Given a room name, get the next open seat index.              *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string name: The name of the room to check.        *
*                                                               *
*------------------------- Return Value ------------------------*
* Return the index of the next open seat. Returns -1 if there   *
* are no open seats, if the room is closed, or if the room      *
* doesn't exist.                                                *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
int ServerConnection::NextOpenSeatInRoom(const std::string name)
{
    if (DoesGameExist(name))
    {
        if(games[name].isOpen)
        {
            for (int i = 0; i < PLAYER_COUNT; i++)
            {
                if(games[name].players[i] == nullptr)
                {
                    return i;
                }
            }
        }
    }
    return -1;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         GetListOfGames                        *
*------------------------- Description -------------------------*
* Get a list of each open game name.                            *
*                                                               *
*------------------------- Parameters --------------------------*
* std::string &list: The string to replace with the list of open*
*   games, formatted for the client to list. If no games are    *
*   present, sets the string to: "<no games>\n"                 *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ServerConnection::GetListOfGames(std::string &list)
{
    list = "";
    if (games.empty())
    {
        list = "<no games>\n";
        return;
    }

    for (auto g : games)
    {
        if (g.second.isOpen)
        {
            list += g.first;
            list += '\n';
        }
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         DoesGameExist                         *
*------------------------- Description -------------------------*
* Given a room name, check if the room exists.                  *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string name: The name of the room to check.        *
*                                                               *
*------------------------- Return Value ------------------------*
* Return true if the game exists.                               *
* Returns false if the game does not exist.                     *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ServerConnection::DoesGameExist(const std::string name)
{
    for (auto g : games)
    {
        if(name.compare(g.first) == 0)
        {
            return true;
        }
    }
    return false;
}

/*===============================================================
||                       Public Functions                      ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          Constructor                          *
*------------------------- Description -------------------------*
* Set up the server's UDP and TCP sockets                       *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
ServerConnection::ServerConnection()
{
    StartServer(udpConnection, tcpConnection);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          Destructor                           *
*------------------------- Description -------------------------*
* Close each client's connection, the UDP socket, and the TCP   *
* soket                                                         *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
ServerConnection::~ServerConnection()
{
    // close client sockets
    for (auto c : clients)
    {
        CloseConnection(c.second.socket);
    }

    //close server sockets
    CloseConnection(udpConnection);
    udpConnection = -1;
    CloseConnection(tcpConnection);
    tcpConnection = -1;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        AcceptNewClient                        *
*------------------------- Description -------------------------*
* Look for a client to connect to the server.                   *
*                                                               *
*------------------------- Return Value ------------------------*
* Return an int representing the client connection.             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
int ServerConnection::AcceptNewClient()
{
    Client newClient;
    newClient.socket = AcceptClient(udpConnection, tcpConnection);
    clients[newClient.socket] = newClient;
    return newClient.socket;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                     InterpretClientRequest                    *
*------------------------- Description -------------------------*
* Given a client connection, wait until they send an action     *
* code. Interpret the code and return what action was requested.*
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*------------------------- Return Value ------------------------*
* Return the action the client requested.                       *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
Action ServerConnection::InterpretClientRequest(const int clientSocket)
{
    char request[CLIENT_ACTION_LENGTH + 1];
    bool stillConnected = ReadDataFromClient(clientSocket, request, CLIENT_ACTION_LENGTH + 1); //LIST_GAME_TIMEOUT_S
    request[CLIENT_ACTION_LENGTH] = '\0';
    if (!stillConnected)
    {
        return UNREGISTER;
    }

    // list 
    if (strcmp(request, LIST_GAME_REQUEST) == 0)
    {
        return LIST;
    }
    // create
    else if (strcmp(request, CREATE_REQUEST) == 0)
    {
        return CREATE;
    }
    // join
    else if (strcmp(request, JOIN_REQUEST) == 0)
    {
        return JOIN;
    }
    //exit
    else if (strcmp(request, EXIT_REQUEST) == 0)
    {
        return EXIT;
    }
    //unregister
    else if (strcmp(request, UNREGISTER_REQUEST) == 0)
    {
        return UNREGISTER;
    }
    //bet
    else if (strcmp(request, BET_REQUEST) == 0)
    {
        return BET;
    }
    //hit
    else if (strcmp(request, HIT_REQUEST) == 0)
    {
        return HIT;
    }
    //stand
    else if (strcmp(request, STAND_REQUEST) == 0)
    {
        return STAND;
    }
    return NONE;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           ListGames                           *
*------------------------- Description -------------------------*
* Send the client a list of all the games available to join.    *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ServerConnection::ListGames(const int clientSocket)
{
    std::string data;
    GetListOfGames(data);
    // send client the list of games
    bool hasSucceeded = true;
    hasSucceeded = SendDataToClient(clientSocket, data.c_str());
    if (hasSucceeded)
    {
        return true;
    }
    return false;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          CreateGame                           *
*------------------------- Description -------------------------*
* Recive a name of a game from the client and create the game if*
* able.                                                         *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ServerConnection::CreateGame(const int clientSocket)
{
    // read name from client
    char buffer[1024];
    bool hasSucceeded = true;
    hasSucceeded = ReadDataFromClient(clientSocket, buffer, ROOM_NAME_BUFFER_SIZE);
    if (!hasSucceeded)
    {
        return false;
    }

    //check if room already exists
    bool validRoom = !DoesGameExist(buffer);
    // make the room and let the client know
    if(validRoom)
    {
        hasSucceeded = SendDataToClient(clientSocket, SERVER_TRUE);
        if (hasSucceeded)
        {
            Game newGame;
            for(int i =0; i < PLAYER_COUNT; i++)
            {
                newGame.players[i] = nullptr;
            }
            newGame.name = buffer;
            games[buffer] = newGame;
            return true;
        }
        return false;
    }
    // let the client know the room is not valid
    else
    {
        SendDataToClient(clientSocket, SERVER_FALSE);
        if (hasSucceeded)
        {
            return true;
        }
        return false;
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           JoinGame                            *
*------------------------- Description -------------------------*
* Recive a name of a game from the client and add them to the   *
* game if able.                                                 *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ServerConnection::JoinGame(const int clientSocket)
{
    // read name from client
    char buffer[1024];
    bool hasSucceeded = true;
    hasSucceeded = ReadDataFromClient(clientSocket, buffer, ROOM_NAME_BUFFER_SIZE);
    if (!hasSucceeded)
    {
        return false;
    }

    // check if room exists
    bool validRoom = DoesGameExist(buffer);

    // check if the room has full players
    int nextSeat;
    if (validRoom)
    {
        nextSeat = NextOpenSeatInRoom(buffer);
        validRoom = (nextSeat != -1);
    }
    
    // add the client to the room and let the client know
    if(validRoom)
    {
        hasSucceeded = SendDataToClient(clientSocket, SERVER_TRUE);
        if (hasSucceeded)
        {
            clients[clientSocket].curGame = &(games[buffer]);
            games[buffer].players[nextSeat] = &(clients[clientSocket]);
            return true;
        }
        return false;
    }
    // let the client know the room is not valid
    else
    {
        SendDataToClient(clientSocket, SERVER_FALSE);
        if(hasSucceeded)
        {
            return true;
        }
        return false;
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           ExitGame                            *
*------------------------- Description -------------------------*
* Remove a client from the game and remove them from the server.*
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ServerConnection::ExitGame(const int clientSocket)
{
    // remove from game
    std::string roomName = clients[clientSocket].curGame->name;
    clients[clientSocket].curGame = nullptr;
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (games[roomName].players[i] != nullptr)
        {
            if (games[roomName].players[i]->socket == clientSocket)
            {
                clients[clientSocket].curGame = nullptr;
                games[roomName].players[i] = nullptr;
            }
        }
        
    }

    // remove from server
    Unregister(clientSocket);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          Unregister                           *
*------------------------- Description -------------------------*
* Remove a client from the server.                              *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ServerConnection::Unregister(const int clientSocket)
{
    // handle player if they are still in game
    if(clients[clientSocket].curGame != nullptr)
    {
        ExitGame(clientSocket);
    }
    // handle player if they are in lobby
    else
    {
        // remove client from list of players
        clients.erase(clientSocket);

        // close client connection
        CloseConnection(clientSocket);
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                              Bet                              *
*------------------------- Description -------------------------*
* Get a bet from the client. I fit's greater than the current   *
* money, bet all their money.                                   *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ServerConnection::Bet(const int clientSocket, int& money)
{
    // read money from client
    char buffer[1024];
    bool hasSucceeded = true;
    hasSucceeded = ReadDataFromClient(clientSocket, buffer, BET_BUFFER_SIZE);
    if (!hasSucceeded)
    {
        return false;
    }
    money = atoi(buffer);

    // If player bet too much, reset to all their money
    if (money > clients[clientSocket].money)
    {
        money = clients[clientSocket].money;
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         SendStateData                         *
*------------------------- Description -------------------------*
* Send the current state of the game to a player.               *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
* const StateData &state: The state of the game.                *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool ServerConnection::SendStateData(const int clientSocket, const StateData &state)
{
    char endOfItem = '_';
    std::string data = "";

    // send playerIndex
    data += std::to_string(state.playerIndex);
    data += endOfItem;
    
    //send playerTurn
    data += std::to_string(state.playerTurn);
    data += endOfItem;

    //send newRound
    if(state.isNewRound)
    {
        data += "1";
        data += endOfItem;
    }
    else
    {
        data += "0";
        data += endOfItem;
    }

    //send player money
    for(int i = 0; i <= PLAYER_COUNT; i++)
    {
        data += std::to_string(state.playerMoney[i]);
        data += endOfItem;
    }

    //send player bets
    for(int i = 0; i <= PLAYER_COUNT; i++)
    {
        data += std::to_string(state.playerBets[i]);
        data += endOfItem;
    }

    //send player hands
    for(int i = 0; i <= PLAYER_COUNT; i++)
    {
        //send number of cards
        data += std::to_string(state.shownCards[i].size());
        data += endOfItem;
        // send each card
        for(int j = 0; j < state.shownCards[i].size(); j++)
        {
            data += state.shownCards[i].at(j);
            data += endOfItem;
        }
    }

    SendDataToClient(clientSocket, data.c_str());
    char response[sizeof(SERVER_TRUE) + 1];
    ReadDataFromClient(clientSocket, response, sizeof(SERVER_TRUE) + 1);
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          GetUserGame                          *
*------------------------- Description -------------------------*
* Find the game a given user is apart of.                       *
*                                                               *
*------------------------- Parameters --------------------------*
* const int clientSocket: The client to handle.                 *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns a pointer to the game a user is apart of.             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
Game* ServerConnection::GetUserGame(const int clientSocket)
{
    return clients[clientSocket].curGame;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          ShutDownGame                         *
*------------------------- Description -------------------------*
* Find the game a given user is apart of.                       *
*                                                               *
*------------------------- Parameters --------------------------*
* Game* game: Disconnect the clients from the given game and    *
* remove the game from the games list.                          *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ServerConnection::ShutDownGame(Game* game)
{
    // unregister each client connected
    for(int i = 0; i < PLAYER_COUNT; i++)
    {
        if(game->players[i] != nullptr)
        {
            ExitGame(game->players[i]->socket);
        }
    }

    // drop the game from the list
    games.erase(game->name);
}
