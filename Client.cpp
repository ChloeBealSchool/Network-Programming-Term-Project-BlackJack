#include "ClientConnection.h"
#include <iostream>
#include <cctype>
#include <vector>
#include <limits>
#include <ios>

//function headers
void DisplayGames(std::string buffer);
std::string StringToLower(const std::string source);
bool HandleUserSelectedGame(std::string buffer, ClientConnection *client);
void ExitGame(ClientConnection *client);
void DisplayPlayer(const int playerIndex, const int money, const int bet, const std::vector<std::string> *cards, const int myIndex);
bool HandleNewGameState(ClientConnection *client, bool &isMyTurn, bool &isNextRound, bool doDisplay = true);
bool HandlePlayerBet(ClientConnection *client);
bool WaitForMyTurn(ClientConnection *client);
bool WaitForNextRound(ClientConnection *client);
bool HandlePlayerTurn(ClientConnection *client);

/*===============================================================
||                            Main                             ||
===============================================================*/

int main()
{
    // connect to server
    class ClientConnection client;
    client.Register();
    bool stillConnected;

    // list games
    char gameList[LIST_GAME_BUFFER_SIZE];
    stillConnected = client.ListGames(gameList);
    if (!stillConnected)
    {
        // Could also occur if no games to read
        //ExitGame(&client);
        //return 0;
    }
    DisplayGames(gameList);

    // Have user pick a game
    stillConnected = HandleUserSelectedGame(gameList, &client);
    if (!stillConnected)
    {
        ExitGame(&client);
        return 0;
    }

    // --- game loop ---
    while (true)
    {
        stillConnected = HandlePlayerBet(&client);
        if (!stillConnected)
        {
            ExitGame(&client);
            return 0;
        }

        stillConnected = WaitForMyTurn(&client);
        if (!stillConnected)
        {
            ExitGame(&client);
            return 0;
        }

        stillConnected = HandlePlayerTurn(&client);
        if (!stillConnected)
        {
            ExitGame(&client);
            return 0;
        }
        
        stillConnected = WaitForNextRound(&client);
        if (!stillConnected)
        {
            ExitGame(&client);
            return 0;
        }
    }
}

/*===============================================================
||                      Helper Functions                       ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          DisplayGames                         *
*------------------------- Description -------------------------*
* Given a list of games, display them to the user.              *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string buffer: The list of games seperated by '\n'.*
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void DisplayGames(std::string buffer)
{
    std::cout << "Please perform one of the following options:" << std::endl;
    std::cout << "1. Type the name of an available room to join it." << std::endl;
    std::cout << "2. Type the name of a non-existing room to create it (Max of 8 characters)." << std::endl;
    std::cout << "3. Type 'exit' to leave the program." << std::endl << std::endl;
    std::cout << "Available Games:" << std::endl;
    std::cout << "=====================================================" << std::endl;
    std::cout << buffer << std::endl;
    std::cout << "=====================================================" << std::endl << std::endl;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         StringToLower                         *
*------------------------- Description -------------------------*
* Conevert a string to lowercase by applying the std::tolower() *
* function on each character.                                   *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string source: The string to copy from.            *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns a lowercase version of the provided string.           *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
std::string StringToLower(const std::string source)
{
    std::string output = "";
    for(int i = 0; i < source.length(); i ++)
    {
        output += std::tolower(source.at(i));
    }
    return output;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                     HandleUserSelectedGame                    *
*------------------------- Description -------------------------*
* Given a list of games and a connection to the server, handle  *
* the user until they join a game or exit from the server.      *
*                                                               *
*------------------------- Parameters --------------------------*
* const std::string buffer: The list of games seperated by '\n'.*
*                                                               *
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool HandleUserSelectedGame(const std::string buffer, ClientConnection *client)
{
    bool waitingForUser = true;
    while (waitingForUser)
    {
        waitingForUser = false;
        // get user input
        std::string userInput;
        std::getline(std::cin, userInput);

        // check for empty string
        if (userInput.compare("") == 0)
        {
            std::cout << "Invalid Input, please try again." << std::endl;
            waitingForUser = true;
        }

        // check for exit 
        else if (StringToLower(userInput).compare("exit") == 0)
        {
            return false;
        }

        // join game
        else if ((buffer.find(StringToLower(userInput)) != std::string::npos) && (buffer.compare("<no games>\n") != 0))
        {
            return client->JoinGame(StringToLower(userInput));
        }

        // create game
        else if ((userInput.length() <= MAX_ROOM_NAME_LENGTH))
        {
            return client->CreateGame(StringToLower(userInput));
        }

        // Invalid input
        else
        {
            std::cout << "Invalid Input, please try again." << std::endl;
            waitingForUser = true;
        }
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           ExitGame                            *
*------------------------- Description -------------------------*
* Exit the server and reset the client state.                   *
*                                                               *
*------------------------- Parameters --------------------------*
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ExitGame(ClientConnection *client)
{
    client->ExitGame();
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         DisplayPlayer                         *
*------------------------- Description -------------------------*
* Helper function to HandleNewGameState().                      *
*                                                               *
* Display a player to the user.                                 *
*                                                               *
*------------------------- Parameters --------------------------*
* const int playerInde: The displayed player's index.           *
*                                                               *
* const int money: The displayed player's money.                *
*                                                               *
* const int bet: The displayed player's bet.                    *
*                                                               *
* const std::vector<std::string> *cards: The displayed player's *
*   shown cards.                                                *
*                                                               *
* const int myIndex: The client's index.                        *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void DisplayPlayer(const int playerIndex, const int money, const int bet, const std::vector<std::string> *cards, const int myIndex)
{
    if (playerIndex == PLAYER_COUNT)
    {
        std::cout << "Dealer" << std::endl;
    }
    else
    {
        //display player title
        std::cout << "Player " << playerIndex;
        if(myIndex == playerIndex)
        {
            std::cout << " (You)";
        }
        std::cout << std::endl;

        //display player money
        std::cout << "\tMoney: " << money << std::endl;

        //display player bet
        std::cout << "\tBet: " << bet << std::endl;
    }

    //displayer player cards
    std::cout << "\tHand: ";
    for (int i = 0; i < cards->size(); i++)
    {
        std::cout << cards->at(i);
        if(i != (cards->size() - 1))
        {
            std::cout << ", ";
        }
    }
    std::cout << std::endl << std::endl;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                       HandleNewGameState                      *
*------------------------- Description -------------------------*
* Recive a new game state from the server and display it to the *
* user. Returns if it's the client's turn, if it's a new round, *
* and if the client is still connected to the server.           *
*                                                               *
*------------------------- Parameters --------------------------*
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
* bool &isMyTurn: Set to true if it's the player's turn. Set to *
*   false if it is not the player's turn.                       *
*                                                               *
* bool &isNextRound: Set to true if it's a new roundn. Set to   *
*   false if it is not a new round.                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool HandleNewGameState(ClientConnection *client, bool &isMyTurn, bool &isNextRound, bool doDisplay)
{
    struct StateData state;
    bool stillConnected = client->GetStateData(state);
    if(!stillConnected)
    {
        return false;
    }

    // get info out of state data
    isMyTurn = (state.playerIndex == state.playerTurn);
    isNextRound = state.isNewRound;
    if (doDisplay)
    {
        std::cout << "______________________________________________________" << std::endl;

        // display data
        for (int i = 0; i <= PLAYER_COUNT; i++)
        {
            DisplayPlayer(i, state.playerMoney[i], state.playerBets[i], &(state.shownCards[i]), state.playerIndex);
        }
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        HandlePlayerBet                        *
*------------------------- Description -------------------------*
* Handle the player until they send a bet greater than 0.       *
*                                                               *
*------------------------- Parameters --------------------------*
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool HandlePlayerBet(ClientConnection *client)
{
    bool waitingForUser = true;
    std::cout << std::endl << "|==================== New Round! ====================|" << std::endl;
    bool temp1;
    bool temp2;
    HandleNewGameState(client, temp1, temp2);
    std::cout << "Enter bet: ";
    while(waitingForUser)
    {
        int bet;
        std::string temp;
        std::cin >> bet;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.clear();


        if (bet > 0)
        {
            bool stillConnected = client->Bet(bet);
            if (stillConnected)
            {
                bool isMyTurn;
                bool isNewRound;
                stillConnected = HandleNewGameState(client, isMyTurn, isNewRound);
                if(!stillConnected)
                {
                    return false;
                }

                // if it's still my turn and a new round, then my bet was invalid
                if (isMyTurn && isNewRound)
                {
                    std::cout << "Invalid bet, try again: ";
                }
                else
                {
                    waitingForUser = false;
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            std::cout << "Invalid bet, try again: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         WaitForMyTurn                         *
*------------------------- Description -------------------------*
* Wait until the client recives a state where they are the      *
* active player.                                                *
*                                                               *
*------------------------- Parameters --------------------------*
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool WaitForMyTurn(ClientConnection *client)
{
    bool stillConnected;
    bool isMyTurn = false;
    bool isNewRound;
    while(!isMyTurn)
    {
        stillConnected = HandleNewGameState(client, isMyTurn, isNewRound);
        if(!stillConnected)
        {
            return false;
        }
    }
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        WaitForNextRound                       *
*------------------------- Description -------------------------*
* Wait until the client recives a state where it's a new round. *
*                                                               *
*------------------------- Parameters --------------------------*
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool WaitForNextRound(ClientConnection *client)
{
    bool stillConnected;
    bool isMyTurn;
    bool isNewRound = false;
    int oldMoney = client->GetPrevMoney();
    while(!isNewRound)
    {
        stillConnected = HandleNewGameState(client, isMyTurn, isNewRound);
        if(!stillConnected)
        {
            return false;
        }
    }
    int newMoney = client->GetPrevMoney();
    if (oldMoney > newMoney)
    {
        std::cout << "You lost!" << std::endl;
    }
    else if (oldMoney < newMoney)
    {
        std::cout << "You win!" << std::endl;
    }
    else
    {
        std::cout << "You tied" << std::endl;
    }
    
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        HandlePlayerBet                        *
*------------------------- Description -------------------------*
* Handle the player until they stand or until they are no longer*
* the active player (bust).
*                                                               *
*------------------------- Parameters --------------------------*
* ClientConnection *client: The client connection to talk to the*
*   server through.                                             *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the server is still active. *
* Returns false if the connection to the server has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool HandlePlayerTurn(ClientConnection *client)
{
    bool isMyTurn;
    bool isNextRound;
    bool stillConnected;
    bool waitingForUser = true;
    bool skipNextStateCheck;
    std::cout << std::endl << "|==================== Your Turn! ====================|" << std::endl;
    stillConnected = HandleNewGameState(client, isMyTurn, isNextRound);
    if(!stillConnected)
    {
        return false;
    }
    
    while (waitingForUser)
    {
        std::cout << "Please perform one of the following options:" << std::endl;
        std::cout << "1. Type 'stand' to end turn." << std::endl;
        std::cout << "2. Type 'hit' to be dealt another card." << std::endl;
        std::cout << "3. Type 'exit' to leave the program." << std::endl << std::endl;
        skipNextStateCheck = false;
        //get user input
        std::string userInput;
        //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, userInput);

        //stand
        if (StringToLower(userInput).compare("stand") == 0)
        {
            stillConnected = client->Stand();
            if(!stillConnected)
            {
                return false;
            }
        }

        // hit
        else if (StringToLower(userInput).compare("hit") == 0)
        {
            stillConnected = client->Hit();
            if(!stillConnected)
            {
                return false;
            }
        }

        //exit
        else if (StringToLower(userInput).compare("exit") == 0)
        {
            return false;
        }

        //invalid input
        else
        {
            std::cout << "Invalid Input, please try again." << std::endl;
            skipNextStateCheck = true;
        }

        if (!skipNextStateCheck)
        {
            stillConnected = HandleNewGameState(client, isMyTurn, isNextRound);
            if(!stillConnected)
            {
                return false;
            }
            if (isMyTurn)
            {
                waitingForUser = true;
            }
            else
            {
                waitingForUser = false;
            }
        }
    }
    return true;
}