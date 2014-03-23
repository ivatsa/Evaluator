
Creating server (compiled binary)
> gcc -o server server.c -lm 


Protocol for client to interact with server
- 1st message from client cs5700spring2013 HELLO <huskyID>
	eg: "cs5700spring2013 HELLO deshpande.sr@husky.neu.edu" 
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
The server is rebuilt to be select based from the previous multi-process 
version. Server is now capable of giving random math puzzle revolving 
around Addition, Subtraction, Multiplication & Division to the client. 

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


