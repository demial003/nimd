Our nim projects uses two files to have users both connect to a nim server, and play the nim game while 
implementing all the of the error messages. in utils.c we made multiple data structures. One for messages, 
one for players. With these two structures we parse through message sent by the user to validate wether if it's 
1. a valid message, 2. the player's turn, 3. a valid move. 
First we create the players, 
then extract the message from players and store it. Once stored we destroy the message by freeing the malloc'd memory. 
Nimd.c keeps track of the active players and uses functions from socket to accept user inut to the server. 
Nimd also keeps a running instance of the game so players can see their moves and current map.
Some invalid moves are moving after being the 2nd person to join so a good test for this is to join second and 
immediately try to move. Yout will be greeted with impatient.