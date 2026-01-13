#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H
#include <string>

#include "ServerAPI.h"
#include <string>
#include <string.h>
#include <unordered_map>
#include <vector>

/*===============================================================
||                       Public Constants                      ||
===============================================================*/
const int PLAYER_COUNT = 2;             // The number of players per room
const int NUM_DECKS = 8;                // The max name of a room name
const int CARDS_IN_STANDARD_DECK = 52;  // The number of cards in a standard deck
// A standard deck of cards
const std::string STANDARD_DECK[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A",
                                    "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A",
                                    "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A",
                                    "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
const int STARTING_MONEY = 100;         // The starting money of a player

/*===============================================================
||                      Public Data Types                      ||
===============================================================*/

// The game state summarized for a player
struct StateData
{
    int playerIndex;    // The index this player is in the arrays
    int playerTurn;     // The index of the active player
    bool isNewRound;    // True: New Round; False: Same Round

    // The money each player has (last spot is dealer, unused)
    int playerMoney[PLAYER_COUNT + 1];
    // The bet each player placed (last spot is dealer, unused)
    int playerBets[PLAYER_COUNT + 1];
    // The hand each player and dealer has
    std::vector<std::string> shownCards[PLAYER_COUNT + 1];
};

// Header so that a pointer can be used in the Client struct
struct Game;

// The connection and game state of a client.
struct Client
{
    // --- server management vars ---
    int socket = -1;            // The client's TCP socket
    Game* curGame = nullptr;    // The client's game

    // --- game management vars ---
    int money = 0;          // The client's money
    int mostRecentBet = 0; // The client's most recent bet
    bool hasStood = false;  // True: Client has stood this round; False: Client has not stood this round;
    bool hasBusted = false; // True: Client has busted this round; False: Client has not busted this round;
    std::string hiddenCard; // The client's face down cards
    std::vector<std::string> shownCards;    // The client's face up cards
};

// A game hosted by the server
struct Game
{
    // --- server management vars ---
    std::string name = "";  // The name of the game
    // The players connected to this game (nullptr if not connected)
    Client **players = new Client*[PLAYER_COUNT];
    bool isOpen = true;     // Is the game available to join

    // --- game management vars ---
    // The deck used to decide what card to deal to each player
    std::string deck[NUM_DECKS * CARDS_IN_STANDARD_DECK];
    // The index of the deck to deal next (reset to 0 after a shuffle)
    int deckIterator = -1;
    // True: Dealer has stood this round; False: Dealer has not stood this round;
    bool hasStood = false;
    // True: Dealer has busted this round; False: Dealer has not busted this round;
    bool hasBusted = false;
    // The dealer's face down cards
    std::string hiddenCard;
    // The dealer's face up cards
    std::vector<std::string> shownCards;
};

// The possible actions a client could request
enum Action {LIST, CREATE, JOIN, EXIT, UNREGISTER, BET, HIT, STAND, NONE};

// A class that handles the server's connections and game states. 
// This including it's TCP socket, UDP socket, client states, 
// and game states.
class ServerConnection
{
    private:
        /*===============================================================
        ||                      Private Constants                      ||
        ===============================================================*/
        const char* SERVER_TRUE = "TTTTTTTT";       // The response from the server if an action was valid
        const char* SERVER_FALSE = "FFFFFFFF";      // The response from the server if an action was invalid

        const char* LIST_GAME_REQUEST = "LISTGAME"; // The client request to see all available games
        const char* CREATE_REQUEST = "CREATEGM";    // The client request to create a game
        const char* JOIN_REQUEST = "JOINGAME";      // The client request to join a game
        const char* EXIT_REQUEST = "EXITGAME";      // The client request to be removed from the game
        const char* UNREGISTER_REQUEST = "UNREGIST";// The client request to unregister from the server
        const char* BET_REQUEST = "BET00000";       // The client request to bet
        const char* HIT_REQUEST = "HIT00000";       // The client request to hit
        const char* STAND_REQUEST = "STAND000";     // The client request to stand

        // The size of a clien't request
        const int CLIENT_ACTION_LENGTH = sizeof(LIST_GAME_REQUEST);

        const int ROOM_NAME_BUFFER_SIZE = 1024;     // The max size to read a room name from a socket
        const int BET_BUFFER_SIZE = 1024;           // The max size to read a bet from a socket

        /*===============================================================
        ||                      Private Variables                      ||
        ===============================================================*/

        // The games the server is hosting. The game name is used as the key
        std::unordered_map<std::string, Game> games;
        // The clients the server is handling. The client socket is used as the key
        std::unordered_map<int, Client> clients;

        int udpConnection;  // The discovery socket for the sever
        int tcpConnection;  // The game socket connection socket


        /*===============================================================
        ||                      Private Functions                      ||
        ===============================================================*/

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                        Copy Constructor                       *
        *------------------------- Description -------------------------*
        * Made private to prevent multiple servers being made within the*
        * same program accidentally.                                    *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ServerConnection(const ServerConnection& a);

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                     Assignment Constructor                    *
        *------------------------- Description -------------------------*
        * Made private to prevent multiple servers being made within the*
        * same program accidentally.                                    *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ServerConnection& operator=(const ServerConnection& other);

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
        int NextOpenSeatInRoom(const std::string name);

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
        void GetListOfGames(std::string &list);

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
        bool DoesGameExist(const std::string name);

    public:
        /*===============================================================
        ||                       Public Functions                      ||
        ===============================================================*/

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          Constructor                          *
        *------------------------- Description -------------------------*
        * Set up the server's UDP and TCP sockets                       *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ServerConnection();

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          Destructor                           *
        *------------------------- Description -------------------------*
        * Close each client's connection, the UDP socket, and the TCP   *
        * soket                                                         *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ~ServerConnection();

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                        AcceptNewClient                        *
        *------------------------- Description -------------------------*
        * Look for a client to connect to the server.                   *
        *                                                               *
        *------------------------- Return Value ------------------------*
        * Return an int representing the client connection.             *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        int AcceptNewClient();

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
        Action InterpretClientRequest(const int clientSocket);

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
        bool ListGames(const int clientSocket);

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
        bool CreateGame(const int clientSocket);

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
        bool JoinGame(const int clientSocket);

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                           ExitGame                            *
        *------------------------- Description -------------------------*
        * Remove a client from the game and remove them from the server.*
        *                                                               *
        *------------------------- Parameters --------------------------*
        * const int clientSocket: The client to handle.                 *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        void ExitGame(const int clientSocket);

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          Unregister                           *
        *------------------------- Description -------------------------*
        * Remove a client from the server.                              *
        *                                                               *
        *------------------------- Parameters --------------------------*
        * const int clientSocket: The client to handle.                 *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        void Unregister(const int clientSocket);

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
        bool Bet(const int clientSocket, int& money);

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
        bool SendStateData(const int clientSocket, const StateData &state);

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
        Game* GetUserGame(const int clientSocket);

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
        void ShutDownGame(Game* game);
};

#endif