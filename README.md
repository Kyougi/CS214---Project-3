# CS214---Project-3
Jason Nguyen/jtn65
Anirudh Chauhan/asc236

The program is designed to implement a server and a client program. The server contains a tic tac toe game within it meant for the client to interact with. The server and client establish a connection with each other to send and recieve input and output. The server is then able to respond to 4 of the protocols from the client and respond with 5 of its own protocols. 2 players from 2 separate clients are able to connect to the server and play a game of tic tac toe through their client by using certain protocols to make their moves. Once in the game, they can either win, loss, ask for a draw, or resign.

The test plan is made to look at each of the breaking points of the program. We used xmit and modified it  The first test plan was to ensure that the client we made for testing could connect to the server. The host used is "localhost" and the port is "5566", with the input for compiling it being: ./ttt localhost 5566
Another test case was to see if the server could handle two players entering the game, meaning 2 terminals connecting to the server to run the game. Messages from the server tells the client to format the input exactly as it states. Clients can also only join one at a time after they enter PLAy and their name. 
Test cases
1. Players with the same name
2. Player with names over 10
3. Having more than 1 game going on at the same time.
4. Ensuring the same person does not move twice
5. Making sure the DRAW method works only when both players agree
6. Making sure MOVE works correctly
7. Making sure MOVD works correctly
9. WAIT is sent to the client when it is supposed to
10. OVER works as intended and 
11. The protocols have the correct number of parameters for each one
12. RSGN works, and then sends OVER to both players
13. BEGN is sent only when 2 players are connected and not before then
14. MOVD is sent to both players after a move
15. INVL is sent when an invalid protocol is asked for
16. The game is able to end correctly and disconnect both clients from the server.



