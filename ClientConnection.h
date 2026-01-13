#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H
#include <vector>   // vector
#include <string>   // string

/*===============================================================
||                       Public Constants                      ||
===============================================================*/
const int PLAYER_COUNT = 2;             // The number of players per room
const int MAX_ROOM_NAME_LENGTH = 8;     // The max name of a room name
const int LIST_GAME_BUFFER_SIZE = 1024; // The max size of the list of games
const int FRAME_BUFFER_SIZE = 1024;     // The max size of a game state

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

// A class that handles the client's connection and game state. 
// This including it's TCP socket, if it's connected to a server,
// and if it's playing a game.
class ClientConnection
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

        const int BET_BUFFER_SIZE = 1024;           // The buffer sized used to convert the client requested money to a character array

        /*===============================================================
        ||                      Private Variables                      ||
        ===============================================================*/
        int tcpConnection;  // The game connection with the server
        bool isRegistered;  // True: Client is connected to server; False: Client is not connected to the server
        bool isInGame;      // True: Client is in a game; False: Client is not in a game
        int prevMoney;      // The last amount of money the client had;

        /*===============================================================
        ||                      Private Functions                      ||
        ===============================================================*/

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                        Copy Constructor                       *
        *------------------------- Description -------------------------*
        * Made private to prevent multiple clients being made within the*
        * same program accidentally.                                    *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ClientConnection(const ClientConnection& a);

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                     Assignment Constructor                    *
        *------------------------- Description -------------------------*
        * Made private to prevent multiple clients being made within the*
        * same program accidentally.                                    *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ClientConnection& operator=(const ClientConnection& other);

    public:
        /*===============================================================
        ||                       Public Functions                      ||
        ===============================================================*/

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          Constructor                          *
        *------------------------- Description -------------------------*
        * Initalizes the private variables.                             *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ClientConnection();

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          Destructor                           *
        *------------------------- Description -------------------------*
        * Exit and Unregisters the client if needed and closes the      *
        * socket.                                                       *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        ~ClientConnection();

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                           Register                            *
        *------------------------- Description -------------------------*
        * Establish a TCP connection with a server.                     *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        void Register();

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
        bool ListGames(char* buffer);

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
        bool CreateGame(const std::string name);

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
        bool JoinGame(const std::string name);

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                           ExitGame                            *
        *------------------------- Description -------------------------*
        * Inform the server that you are disconnecting from the game.   *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        void ExitGame();

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          Unregister                           *
        *------------------------- Description -------------------------*
        * Inform the server that you are disconnecting from the lobby.  *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
        void Unregister();

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
        bool Bet(const int val);

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
        bool Hit();

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
        bool Stand();

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
        bool GetStateData(StateData &state);

        /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*
        *                          GetPrevMoney                         *
        *------------------------- Description -------------------------*
        * Get the last amount of money the client had.                  *
        *                                                               *
        *------------------------- Return Value ------------------------*
        * Returns the amount of money the player had last.              *
        *                                                               *
        *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
       int GetPrevMoney();
};

#endif