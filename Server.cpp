#include "ServerConnection.h"
#include <pthread.h>
#include <iterator>
#include <random>
#include <algorithm>
#include <iostream>

//thread function headers
void *LobbyRoom(void *arg);
void *GameRoom(void *arg);

// helper function headers
void FillDeck(Game *game);
void ShuffleDeck(Game *game);
void DealCardToPlayer(Game *game, Client *player);
void RevealHiddenCard(Client *player);
void DealStartingHands(Game *game);
void SetBets(Game *game, ServerConnection *server);
int GetPlayerScore(Client *player);
int GetDealerScore(Game *game);
void RunPlayer(Game *game, Client *player, ServerConnection *server);
void RunDealer(Game* game, ServerConnection *server);
void PayoutPlayer(Game *game, Client *player);
bool SendStateToPlayer(ServerConnection *server, Game *game, const bool isNewRound, const int playerTurn, Client *player);
void SendStateToAllPlayers(ServerConnection *server, Game *game, const bool isNewRound, const int playerTurn);

/*===============================================================
||                          Constants                          ||
===============================================================*/
const int DEALER_STAND_ON = 17; // The value the dealer will stand on
const int MAX_SAFE_SCORE = 21;  // The highets score before a bust

/*===============================================================
||                      Custom Data Types                      ||
===============================================================*/
struct LobbyData
{
    int clientSocket;
    class ServerConnection *server;
};

struct GameData
{
    class ServerConnection *server;
    Game *game;
};

/*===============================================================
||                            Main                             ||
===============================================================*/
int main()
{
    std::cout << "Staring Server" << std::endl;
    //make a server connection
    class ServerConnection server;
    while(1)
    {
        // wait for a client to connect
        int newSocket = server.AcceptNewClient();
        std::cout << "Server found Client: " << newSocket << std::endl;
    
        // once a client connects, place them on a lobby thread
        pthread_t newLobby;
        struct LobbyData *data = new LobbyData;
        data->clientSocket = newSocket;
        data->server = &server;
        pthread_create(&newLobby, NULL, LobbyRoom, (void *) data);
    }

}

/*===============================================================
||                      Thread Functions                       ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          *LobbyRoom                           *
*------------------------- Description -------------------------*
* A thread to handle the user until they join a game.           *
*                                                               *
*------------------------- Parameters --------------------------*
* void *arg: The data used to handle the client in the lobby.   *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void *LobbyRoom(void *arg)
{
    // format thread data
    struct LobbyData *data = (struct LobbyData *)arg;

    // wait for user to select a room
    bool waitingOnUser = true;
    bool disconnected = false;
    bool noErrors;
    bool newRoom = false;
    do
    {
        
        Action userInput = data->server->InterpretClientRequest(data->clientSocket);
        switch (userInput)
        {
            case (LIST):
                noErrors = data->server->ListGames(data->clientSocket);
                if (!noErrors)
                {
                    waitingOnUser = false;
                    disconnected = true;
                }
                break;
            
            case (CREATE):
                std::cout << "Server creating game" << std::endl;
                noErrors = data->server->CreateGame(data->clientSocket);
                if (!noErrors)
                {
                    waitingOnUser = false;
                    disconnected = true;
                }
                newRoom = true;
                break;

            case (JOIN):
                std::cout << "Server putting Client in game" << std::endl;
                noErrors = data->server->JoinGame(data->clientSocket);
                if (!noErrors)
                {
                    disconnected = true;
                }
                waitingOnUser = false;
                break;

            case (UNREGISTER):
                waitingOnUser = false;
                disconnected = true;
                break;

            default:
                break;
        }
    } while (waitingOnUser);

    // if the user disconnected, don't do anything
    if(disconnected)
    {
        data->server->Unregister(data->clientSocket);
        return 0;
    }

    // if a clients makes a new room, create a new room thread
    if (newRoom)
    {
        pthread_t newGame;
        struct GameData *gameData = new GameData;
        gameData->server = data->server;
        gameData->game = data->server->GetUserGame(data->clientSocket);
        pthread_create(&newGame, NULL, GameRoom, (void *) gameData);
    }
    return 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           *GameRoom                           *
*------------------------- Description -------------------------*
* A thread to run a game for clients equal to PLAYER_COUNT.     *
*                                                               *
*------------------------- Parameters --------------------------*
* void *arg: The data used to handle the client in the lobby.   *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void *GameRoom(void *arg)
{
    // format thread data
    struct GameData *data = (struct GameData *)arg;

    // wait till all users are ready to play
    bool waitForUser;
    do
    {
        waitForUser = false;
        for (int i = 0; i < PLAYER_COUNT; i++)
        {
            if (data->game->players[i] == nullptr)
            {
                waitForUser = true;
            }
        }
    } while(waitForUser);
    std::cout << "Starting Game" << std::endl;
    data->game->isOpen = false;

    // --- Set up game ---
    // init deck
    FillDeck(data->game);
    ShuffleDeck(data->game);

    // init player money
    for(int i = 0; i < PLAYER_COUNT; i++)
    {
        data->game->players[i]->money = STARTING_MONEY;
    }

    // Game Loop
    //SendStateToAllPlayers(data->server, data->game, false, -1);
    bool hasPlayers = true;
    do
    {
        // get bets
        SetBets(data->game, data->server);
        SendStateToAllPlayers(data->server, data->game, false, -1);

        // deal
        DealStartingHands(data->game);
        SendStateToAllPlayers(data->server, data->game, false, -1);

        // handle each user action
        for (int i = 0; i < PLAYER_COUNT; i ++)
        {
            if (data->game->players[i] != nullptr)
            {
                RunPlayer(data->game, data->game->players[i], data->server);
            }
        }

        // play dealer
        std::cout << "Dealer Playing" << std::endl;
        RunDealer(data->game, data->server);

        // for each winner, pay out, for each loser, lose money
        std::cout << "Paying out" << std::endl;
        for (int i = 0; i < PLAYER_COUNT; i ++)
        {
            if (data->game->players[i] != nullptr)
            {
                PayoutPlayer(data->game, data->game->players[i]);
            }
        }
        SendStateToAllPlayers(data->server, data->game, true, -1);

        // For each player with no money, give them some pity money
        std::cout << "Pitty money" << std::endl;
        for (int i = 0; i < PLAYER_COUNT; i ++)
        {
            if (data->game->players[i] != nullptr)
            {
                if(data->game->players[i]->money <= 0 )
                {
                    data->game->players[i]->money = 10;
                }
            }
        }

        // remove cards from all players and dealer
        std::cout << "discarding hands" << std::endl;
        for (int i = 0; i < PLAYER_COUNT; i ++)
        {
            if (data->game->players[i] != nullptr)
            {
                data->game->players[i]->shownCards.clear();
            }
        }
        data->game->shownCards.clear();

        //shuffle deck if below half way
        
        if (data->game->deckIterator >= (NUM_DECKS*CARDS_IN_STANDARD_DECK / 2))
        {
            std::cout << "Shuffling deck" << std::endl;
            ShuffleDeck(data->game);
        }

        // Reset player bet to 0
        for (int i = 0; i < PLAYER_COUNT; i ++)
        {
            if (data->game->players[i] != nullptr)
            {
                data->game->players[i]->mostRecentBet = 0;
            }
        }

        // Check if users are still in room
        std::cout << "Making sure there are still players" << std::endl;
        hasPlayers = false;
        for (int i = 0; i < PLAYER_COUNT; i++)
        {
            if (data->game->players[i] != nullptr)
            {
                hasPlayers = true;
            }
        }
    } while (hasPlayers);
    std::cout << "Shutting down game" << std::endl;
    data->server->ShutDownGame(data->game);
    return 0;
}

/*===============================================================
||                      Helper Functions                       ||
===============================================================*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           FillDeck                            *
*------------------------- Description -------------------------*
* Fill the game deck with cards.                                *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to fill the deck for.                    *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void FillDeck(Game *game)
{
    game->deckIterator = 0;
    for (int i = 0; i < NUM_DECKS; i++)
    {
        for (int j = 0; j < CARDS_IN_STANDARD_DECK; j++)
        {
            
            game->deck[game->deckIterator] = STANDARD_DECK[j];
            game->deckIterator++;
        }
        
    }
    game->deckIterator = 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          ShuffleDeck                          *
*------------------------- Description -------------------------*
* Shuffle the game deck.                                        *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to shuffle the deck for.                 *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void ShuffleDeck(Game *game)
{
    std::random_device rd;
    std::mt19937 g(rd());
 
    std::shuffle(std::begin(game->deck), std::end(game->deck), g);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        DealCardToPlayer                       *
*------------------------- Description -------------------------*
* Deal a card to the player.                                    *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to deal from.                            *
*                                                               *
* Client *player: The player to deal to.                        *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void DealCardToPlayer(Game *game, Client *player)
{
    player->shownCards.push_back(game->deck[game->deckIterator]);
    game->deckIterator++;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                        RevealHiddenCard                       *
*------------------------- Description -------------------------*
* Reveal the player's hidden card                               *
*                                                               *
*------------------------- Parameters --------------------------*
* Client *player: The player to reveal the card of.             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void RevealHiddenCard(Client *player)
{
    player->shownCards.push_back(player->hiddenCard);
    player->hiddenCard = "";
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                       DealStartingHands                       *
*------------------------- Description -------------------------*
* Deal the starting hands to each player and the dealer.        *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to deal the starting hands of.           *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void DealStartingHands(Game *game)
{
    // deal to players
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (game->players[i] != nullptr)
        {
            //deal hidden card
            game->players[i]->hiddenCard = game->deck[game->deckIterator];
            game->deckIterator++;

            //deal shown card
            DealCardToPlayer(game, game->players[i]);
        }
    }

    // deal to dealer
    // deal hidden card
    game->hiddenCard = game->deck[game->deckIterator];
    game->deckIterator++;

    // deal shown card
    game->shownCards.push_back(game->deck[game->deckIterator]);
    game->deckIterator++;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                            SetBets                            *
*------------------------- Description -------------------------*
* Wait for each player to bet and store it.                     *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to get the bets from the players of.     *
*                                                               *
* ServerConnection *server: The server connection to talk to the*
*   client through.                                             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void SetBets(Game *game, ServerConnection *server)
{
    SendStateToAllPlayers(server, game, true, -1);
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (game->players[i] != nullptr)
        {
            // wait for user to make a bet
            bool waitingOnUser = true;
            bool disconnected = false;
            bool leftGame = false;
            bool noErrors;
            do
            {
                Action userInput = server->InterpretClientRequest(game->players[i]->socket);
                switch (userInput)
                {
                    case (BET):
                        // get requested money
                        int betMoney;
                        noErrors = server->Bet(game->players[i]->socket, betMoney);
                        if (!noErrors)
                        {
                            waitingOnUser = false;
                            disconnected = true;
                        }
                        // check if it was a valid request
                        else if( betMoney > 0)
                        {
                            waitingOnUser = false;
                            game->players[i]->mostRecentBet = betMoney;
                        }
                        else
                        {
                            SendStateToPlayer(server, game, true, i, game->players[i]);
                        }
                        break;
                    
                    case (EXIT):
                        leftGame = true;
                        waitingOnUser = false;
                        disconnected = true;
                        if (!noErrors)
                        break;

                    case (UNREGISTER):
                        server->Unregister(game->players[i]->socket);
                        waitingOnUser = false;
                        disconnected = true;
                        break;

                    default:
                        break;
                }
            } while (waitingOnUser);
        }
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         GetPlayerScore                        *
*------------------------- Description -------------------------*
* Get the score of the player.                                  *
*                                                               *
*------------------------- Parameters --------------------------*
* Client *player: The player to get the score of.               *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns the player's score.                                   *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
int GetPlayerScore(Client *player)
{
    int playerScore = 0;
    int numAces = 0;
    // convert cards to score
    for ( std::string cardStr : player->shownCards)
    {
        if (cardStr.compare("2") == 0)
        {
            playerScore += 2;
        }
        else if (cardStr.compare("3") == 0)
        {
            playerScore += 3;
        }
        else if (cardStr.compare("4") == 0)
        {
            playerScore += 4;
        }
        else if (cardStr.compare("5") == 0)
        {
            playerScore += 5;
        }
        else if (cardStr.compare("6") == 0)
        {
            playerScore += 6;
        }
        else if (cardStr.compare("7") == 0)
        {
            playerScore += 7;
        }
        else if (cardStr.compare("8") == 0)
        {
            playerScore += 8;
        }
        else if (cardStr.compare("9") == 0)
        {
            playerScore += 9;
        }
        else if (cardStr.compare("10") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("J") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("Q") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("K") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("A") == 0)
        {
            numAces++;
            playerScore += 1;
        }
    }

    //handle aces
    for (int i = 0; i < numAces; i++)
    {
        // for each aces, see if changing to an 11 will stay under 21
        if(playerScore + 9 <= MAX_SAFE_SCORE)
        {
            playerScore += 10;
        }
        else
        {
            i = numAces;
        }
    }
    return playerScore;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                         GetDealerScore                        *
*------------------------- Description -------------------------*
* Get the score of the dealer.                                  *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to get the dealer score of.              *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns the player's score.                                   *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
int GetDealerScore(Game *game)
{
    int playerScore = 0;
    int numAces = 0;
    // convert cards to score
    for ( std::string cardStr : game->shownCards)
    {
        if (cardStr.compare("2") == 0)
        {
            playerScore += 2;
        }
        else if (cardStr.compare("3") == 0)
        {
            playerScore += 3;
        }
        else if (cardStr.compare("4") == 0)
        {
            playerScore += 4;
        }
        else if (cardStr.compare("5") == 0)
        {
            playerScore += 5;
        }
        else if (cardStr.compare("6") == 0)
        {
            playerScore += 6;
        }
        else if (cardStr.compare("7") == 0)
        {
            playerScore += 7;
        }
        else if (cardStr.compare("8") == 0)
        {
            playerScore += 8;
        }
        else if (cardStr.compare("9") == 0)
        {
            playerScore += 9;
        }
        else if (cardStr.compare("10") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("J") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("Q") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("K") == 0)
        {
            playerScore += 10;
        }
        else if (cardStr.compare("A") == 0)
        {
            numAces++;
            playerScore += 1;
        }
    }

    //handle aces
    for (int i = 0; i < numAces; i++)
    {
        // for each aces, see if changing to an 11 will stay under 21
        if(playerScore + 9 <= MAX_SAFE_SCORE)
        {
            playerScore += 10;
        }
        else
        {
            i = numAces;
        }
    }
    return playerScore;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           RunPlayer                           *
*------------------------- Description -------------------------*
* Run the player until they bust or stand.                      *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to run.                                  *
*                                                               *
* Client *player: The player to run.                            *
*                                                               *
* ServerConnection *server: The server connection to talk to the*
*   client through.                                             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void RunPlayer(Game *game, Client *player, ServerConnection *server)
{
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (game->players[i] != nullptr)
        {
            if (game->players[i]->socket == player->socket)
            {
                SendStateToAllPlayers(server, game, false, i);
            }
        }
    }
    player->hasBusted = false;
    player->hasStood = false;
    RevealHiddenCard(player);
    // wait for user to make stand or bust
    bool waitingOnUser = true;
    do
    {
        // check if user has busted
        int playerScore = GetPlayerScore(player);
        std::cout << "Player score: " << playerScore << std::endl;
        if (playerScore > MAX_SAFE_SCORE)
        {
            player->hasBusted = true;
            waitingOnUser = false;
        }
        else
        {
            // send state to all palyers
            for (int i = 0; i < PLAYER_COUNT; i++)
            {
                if (game->players[i] != nullptr)
                {
                    if (game->players[i]->socket == player->socket)
                    {
                        SendStateToAllPlayers(server, game, false, i);
                    }
                }
            }
            Action userInput = server->InterpretClientRequest(player->socket);
            switch (userInput)
            {
                case (HIT):
                    DealCardToPlayer(game, player);
                    break;

                case (STAND):
                    waitingOnUser = false;
                    player->hasStood = true;
                    break;
                
                case (EXIT):
                    server->Unregister(player->socket);
                    waitingOnUser = false;
                    break;

                case (UNREGISTER):
                    server->Unregister(player->socket);
                    waitingOnUser = false;
                    break;

                default:
                    break;
            }
        }
    } while (waitingOnUser);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                           RunDealer                           *
*------------------------- Description -------------------------*
* Run the dealer until they bust or stand.                      *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game to run the dealer for.                   *
*                                                               *
* ServerConnection *server: The server connection to talk to the*
*   client through.                                             *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void RunDealer(Game* game, ServerConnection *server)
{
    SendStateToAllPlayers(server, game, false, -1);
    game->hasBusted = false;
    game->hasStood = false;
    // reveal hidden card
    game->shownCards.push_back(game->hiddenCard);
    game->hiddenCard = "";

    // play game
    bool playing = true;
    do
    {
        // check if dealer has busted
        int dealerScore = GetDealerScore(game);
        if (dealerScore > MAX_SAFE_SCORE)
        {
            game->hasBusted = true;
            playing = false;
        }
        // run dealer logic
        else
        {
            // If the dealer hasn't reched thier limit, hit
            if (dealerScore < DEALER_STAND_ON)
            {
                game->shownCards.push_back(game->deck[game->deckIterator]);
                game->deckIterator++;
            }
            // dealer stands
            else
            {
                game->hasStood = true;
                playing = false;
            }
        }
    } while (playing);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                          PayoutPlayer                         *
*------------------------- Description -------------------------*
* Payout the player.                                            *
*                                                               *
*------------------------- Parameters --------------------------*
* Game *game: The game with the dealer to compare against.      *
*                                                               *
* Client *player: The player to run to payout.                  *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void PayoutPlayer(Game *game, Client *player)
{
    // find if player lost
    bool lost = false;
    //if player busted, no money
    if (player->hasBusted)
    {
        lost = true;
    }
    //if dealer busted and player did not, yay money
    else if(game->hasBusted)
    {
        lost = false;
    }
    // if dealer and player both did not bust, and player has a lower score, no money
    else if (GetDealerScore(game) > GetPlayerScore(player))
    {
        lost = true;
    }
    // if dealer and player both did not bust, and player has better score, yay money
    else if (GetDealerScore(game) < GetPlayerScore(player))
    {
        lost = false;
    }
    // else, there is a tie, do nothing
    else
    {
        return;
    }

    //if player lost, lose money
    if (lost)
    {
        player->money -= player->mostRecentBet;
    }
    // if player won, gain money
    else
    {
        //check for blackjack add an extra 50%
        if ((GetPlayerScore(player) == MAX_SAFE_SCORE) && (player->shownCards.size() == 2))
        {
            player->money += (player->mostRecentBet / 2);
        }
        player->money += player->mostRecentBet;
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                       SendStateToPlayer                       *
*------------------------- Description -------------------------*
* Send the current game state to the player.                    *
*                                                               *
*------------------------- Parameters --------------------------*
* ServerConnection *server: The server connection to talk to the*
*   client through.                                             *
*                                                               *
* Game *game: The game to send.                                 *
*                                                               *
* const bool isNewRound: If the state is a new round.           *
*                                                               *
* const int playerTurn: The index of the active player.         *
*                                                               *
* Client *player: The player to send the state to.              *
*                                                               *
*------------------------- Return Value ------------------------*
* Returns true if the connection to the client is still active. *
* Returns false if the connection to the client has terminated. *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
bool SendStateToPlayer(ServerConnection *server, Game *game, const bool isNewRound, const int playerTurn, Client *player)
{
    // set up state packet
    struct StateData state;

    // is new round
    state.isNewRound = isNewRound;
    // player turn
    state.playerTurn = playerTurn;

    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        // player index
        if (game->players[i] == player)
        {
            state.playerIndex = i;
        }
        if (game->players[i] != nullptr)
        {
            // player's money
            state.playerMoney[i] = game->players[i]->money;

            // player's bets
            state.playerBets[i] = game->players[i]->mostRecentBet;

            // player's cards
            state.shownCards[i] = game->players[i]->shownCards;
        }
        else
        {
            // player's money
            state.playerMoney[i] = -1;

            // player's bets
            state.playerBets[i] = -1;

            // player's cards
            state.shownCards[i] = std::vector<std::string>();
        }
    }

    // add dealer
    state.playerMoney[PLAYER_COUNT] = -1;
    state.playerBets[PLAYER_COUNT] = -1;
    state.shownCards[PLAYER_COUNT] = game->shownCards;

    // send state packet to client
    return server->SendStateData(player->socket, state);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
*                     SendStateToAllPlayers                     *
*------------------------- Description -------------------------*
* Send the current game state to all players.                   *
*                                                               *
*------------------------- Parameters --------------------------*
* ServerConnection *server: The server connection to talk to the*
*   clients through.                                            *
*                                                               *
* Game *game: The game to send.                                 *
*                                                               *
* const bool isNewRound: If the state is a new round.           *
*                                                               *
* const int playerTurn: The index of the active player.         *
*                                                               *
*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void SendStateToAllPlayers(ServerConnection *server, Game *game, const bool isNewRound, const int playerTurn)
{
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (game->players[i] != nullptr)
        {
            bool stillConnected = SendStateToPlayer(server, game, isNewRound, playerTurn, game->players[i]);
            if (!stillConnected)
            {
                server->Unregister(game->players[i]->socket);
            }
        }
    }
}