# Tic_Tac_Toe_Socket_Programming

1) C++ Socket programming for building a tic-tac-toe server and client.
2) The server is responsible for pairing the players and supports multiple games simultaneously by using POSIX threads.
3) Handles edge cases like abrupt disconnections by the client, timeouts, etc.
4) For starting the Server:-
   ```
   g++ -pthread server.cpp tictactoe.cpp  -o server
   ./server
   ```
5) For connecting the server as a client :-
   ```
   g++ -pthread client.cpp -o client
   ./client <IP address of server>
   ```   
