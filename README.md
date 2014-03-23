EVALUATOR
This is a C implementation of server program, to which multiple clients can connect and server does tests on the client program and gives out a secret key. This secret key will be the base of evaluator (Professor) to give out grades to the student client implementation.


Creating server (compiled binary)
> gcc -o server server.c -lm 


Protocol for client to interact with server
- 1st message from client cs5700spring2013 HELLO <huskyID>
	eg: "cs5700spring2013 HELLO xyz@husky.neu.edu" 
	server's response: "cs5700spring2013 STATUS 0 - 235 <IP>:<port>" 

- 2nd message onwards.. cs5700spring2013 <solution to math puzzle>
	eg: "cs5700spring2013 -235" 
	server's response: "cs5700spring2013 STATUS 999 * 235 <IP>:<port>" 
. 
. 
. 

- Finally Server sends a BYE message which has secret code (fixed to be 20
  characters) and terminates the socket. 
	server's response: "cs5700spring2013 12345678901234567890 BYE" 


Details of server 
The server is rebuilt to be SELECT-based from the previously multi-process 
version. Server is capable of giving random math puzzle revolving 
around Addition, Subtraction, Multiplication & Division to the client.
These puzzles make sure the client program is able to make connection
properly and obey to commands given by the server.

The following are the peculiarities/qualities of the server: 
- Server ends connection with the client on wrong solution to puzzle. 
- Total number of math puzzle rounds will be between 100 to 999 
  (randomly picked). 
- In case of division, the resulting number is truncated to integer 
  value (floor function). 
- In case of division, 2nd random number will never be zero. 
- In case of Subtraction, Negative integer results are valid. 
- File name of file containing secret codes is set to be "database" 
  (format is as you specified in specification email) .
- The garbage collection happens every 60 seconds, the server looks for 
  the sockets which had not been active for more than 30 seconds and 
  terminates the connection.
