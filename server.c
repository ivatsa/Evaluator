#include <stdio.h>
#include <string.h>	/* for using strtok, strcmp, etc */
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h> /* for using atoi */
#include <pthread.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include "common.h"

fd_set readfds;
struct client *root;
struct sockaddr_in cli;
char client_addr[100];
int listening_sd, fdMax = 0;
char secret_code[25];


int clientFd[10];

struct client {
	int sd; //socket descriptor
	int rand1; 
	int rand2;
	int operator;
	int roundNumber;
	struct sockaddr_in cli;  //char client_addr[50];
	char studentID[50];
	int rounds;
	time_t lastActive;
	struct client *next;
};

/*
 * This function is called when a system call fails.
 * It displays a message about the error on stderr and then aborts the program.
 * The perror man page gives more information.
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void updateCode(struct client *conductor)
{
	char studentID[50];
	char line[80];

	FILE *fp;

	if( (fp = fopen("./database", "r+")) == NULL)
	{
    		printf("No such file\n");
    		exit(1);
	}  

	if (fp == NULL)
	{
		printf("Error Reading File\n");
	}

	while(fgets(line, 80, fp) != NULL)	//limiting read to 80 chars
	{
		sscanf (line,"%s %s",secret_code,studentID);
		if(strcmp(studentID, conductor->studentID)==0) {
			 return;
		}
	}
	strcpy(secret_code,"12345678901234567890");
	return;	//default code, not a valid Husky ID
}

char getOperator(int op)
{
	switch (op) {
		case 0:
			return '+';
		case 1:
			return '-';
		case 2:
			return '*';
		case 3:
			return '/';
		//any execptions
		default:
			return '+';
	}
}

long int getSolution(struct client *conductor)
{
	double x;
	switch (conductor->operator) {
		case 0:
			return conductor->rand1 + conductor->rand2;
		case 1:
			return conductor->rand1 - conductor->rand2;
		case 2:
			return conductor->rand1 * conductor->rand2;
		case 3:
			return conductor->rand1 / conductor->rand2;	//during return int conversion truncates decimal part
		//any execptions
		default:
			return conductor->rand1 + conductor->rand2;
	}
}

void generateRandom(struct client *conductor)
{
	conductor->rand1 = atoi(strtok(inet_ntoa(conductor->cli.sin_addr), "."));
	conductor->rand1 += atoi(strtok(NULL, "."));
	conductor->rand1 += atoi(strtok(NULL, "."));
	conductor->rand1 += atoi(strtok(NULL, "."));
	conductor->rand1 += (conductor->rand1*13*conductor->roundNumber) % 1000;
	conductor->rand1 *= (int)cli.sin_port;
	conductor->rand1 %= 1000;
	conductor->rand2 = (conductor->rand1 ^ (conductor->rand1 * 3223 * conductor->roundNumber)) % 1000;
	
	conductor->operator = rand() % 4;	//for oprator shuffling, not actually used yet
	
	if(conductor->operator == 3 && conductor->rand2 == 0)
		conductor->rand2 += 1;
}

void newClient(struct sockaddr_in cli_struct, int sock_des)
{
    struct client *conductor;  
    conductor = root;
    if ( conductor != NULL ) {
        while ( conductor->next != NULL)
        {
            conductor = conductor->next;
        }
    }
    /* Creates a client at the end of the list */
    conductor->next = (struct client *)malloc( sizeof(struct client) );  
    conductor = conductor->next; 

    if ( conductor == NULL )
    {
        error( "Out of memory" );
    }
    /* initialize the new memory */
	conductor->next = NULL;         
	conductor->sd = sock_des;
	conductor->rand1 = 0; 
	conductor->rand2 = 0;
	conductor->operator = 0;
	conductor->roundNumber = 1;
	conductor->cli = cli_struct;
	conductor->rounds = 0;
	conductor->lastActive = time (NULL);
	memset(&conductor->studentID, 0, 50);	//student has not yet provided his identity		`
	return;
}

void delClient(int sd)
{
	struct client *conductor, *prev;
	prev = NULL;
	conductor = root;
		while ( conductor != NULL)
		{
			if(conductor->sd == sd) {
				prev->next = conductor->next;
				//printf("Connection to a client closed! \n");
				fflush(stdout);
				FD_CLR(sd,&readfds);
				free(conductor);
				close(sd);
				break;
			}
			prev = conductor;
			conductor = conductor->next;
	}
	return;
}

int serve_client(struct client *conductor, char *client_addr)
{
	char  client_msg1[MAX_STR_SIZE] = "";
	char  client_msg2[MAX_STR_SIZE] = "";
	char  server_msg1[MAX_STR_SIZE] = "";
	char  server_msg2[MAX_STR_SIZE] = "";
	char  magic_str[] = MAGIC_STRING;
	char delims[] = " \r\n";	/* delimiters, '\r' is necessary if client is using telnet */
	char eol_delims[] = "\r\n";	/* delimiters, '\r' is necessary if client is using telnet */
	char *token = NULL;
	int readbytes;
	char *name = NULL;
	char rand_num_str[100];
	char rand_num_str2[100];
	char rand_num_str_total[100];
	
	if(conductor->roundNumber == 1)
	{	
	/* first message, e.g. "cs5700spring2013 HELLO cs417002 Kan-Leung" */
		readbytes = read(conductor->sd, client_msg1, MAX_STR_SIZE);
	
		if (readbytes==-1) return -1;
		
		token = strtok(client_msg1, delims);
		if (token==NULL || strcmp(token, magic_str)!=0) return -1;
		
		token = strtok(NULL, delims);
		if (token==NULL || strcmp(token, "HELLO")!=0) return -1;
		
		token = strtok(NULL, delims);
		if (token==NULL) return -1;
		
		name = token;
		sprintf(conductor->studentID, "%s", name); //store email ID
	
		if (strtok(NULL, delims)!=NULL) return -1;

		generateRandom(conductor);
		conductor->rounds = 100 + conductor->rand1 % 900;	//fix total rounds for once

		sprintf(rand_num_str, "%d", conductor->rand1);
		sprintf(rand_num_str2, "%d", conductor->rand2);

		sprintf(server_msg1,"%s STATUS %s %c %s %s\n", magic_str, rand_num_str, getOperator(conductor->operator), rand_num_str2, client_addr);
		write(conductor->sd, server_msg1, strlen(server_msg1));
		conductor->roundNumber++;
		conductor->lastActive = time (NULL);
		return 0;
	}
	if (conductor->roundNumber > 1 && conductor->roundNumber <= conductor->rounds)
	{
	/* second message onwards, e.g. "cs5700spring2013 12345" */
		sprintf(rand_num_str_total, "%ld", getSolution(conductor));
		readbytes = read(conductor->sd, client_msg2, MAX_STR_SIZE);
		if (readbytes==-1) return -1;
		
		token = strtok(client_msg2, delims);
		if (token==NULL || strcmp(token, magic_str)!=0) return -1;
		
		// BYE is now initiated by server 
		//token = strtok(NULL, delims);
		//if (token==NULL || strcmp(token, "BYE")!=0) return -1;
		
		token = strtok(NULL, delims);
		if (token==NULL || strcmp(token, rand_num_str_total)!=0) return -1;
		
		generateRandom(conductor);
		sprintf(rand_num_str, "%d", conductor->rand1);
		sprintf(rand_num_str2, "%d", conductor->rand2);

		sprintf(server_msg1,"%s STATUS %s %c %s %s\n", magic_str, rand_num_str, getOperator(conductor->operator), rand_num_str2, client_addr);
		write(conductor->sd, server_msg1, strlen(server_msg1));
		conductor->roundNumber++;
		conductor->lastActive = time (NULL);
		return 0;
	}
	sprintf(rand_num_str_total, "%ld", getSolution(conductor));
	readbytes = read(conductor->sd, client_msg2, MAX_STR_SIZE);
	//verify, Give out the secret code & say BYE.
	if (readbytes==-1) return -1;
	
	token = strtok(client_msg2, delims);
	if (token==NULL || strcmp(token, magic_str)!=0) return -1;
	
	token = strtok(NULL, delims);
	if (token==NULL || strcmp(token, rand_num_str_total)!=0) return -1;
	updateCode(conductor);
	sprintf(server_msg2, "%s %s BYE\n", magic_str, secret_code);
	write(conductor->sd, server_msg2, strlen(server_msg2));
	delClient(conductor->sd);	
	return 0;
}


void removeDeadSocket()
{
	struct client *conductor;  
	conductor = root->next; 
		while ( conductor != NULL)
		{
			if ((long int)time(0) - (long int)conductor->lastActive > 30)
				delClient(conductor->sd);
			conductor = conductor->next;
		}
}


void serveRequest(struct client *conductor)
{
	if(conductor->sd == listening_sd)
	{
		int sin_size = sizeof(struct sockaddr_in);
		int client_sock = accept(listening_sd, (struct sockaddr *)&cli, &sin_size);
		if (client_sock == -1)
			error("accept");

		newClient(cli, client_sock);
		FD_SET(client_sock,&readfds);
		if(client_sock > fdMax)
			fdMax = client_sock;
		sprintf(client_addr, "%s:%d", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
		printf("\nconnected to:  %s", client_addr);
		fflush(stdout);
		return;
	}
	fflush(stdout);
	sprintf(client_addr, "%s:%d", inet_ntoa(conductor->cli.sin_addr), ntohs(conductor->cli.sin_port));
	if (serve_client(conductor, client_addr) == -1)
	{
		printf("\n**Error** from %s", client_addr);
		fflush(stdout);
		delClient(conductor->sd);
	}
	return;
}


int main(int argc, char **argv)
{
	srand(time(0));
	struct sockaddr_in sin;
	int ret_val;
	struct timeval timeout;  /* Timeout for select */
	int selectResponse;
	FD_ZERO(&readfds);

	/* Verify command-line arguments */
	//struct hostent *h = (argc > 1) ? gethostbyname(argv[1]) : gethostbyname(SERVER_HOSTNAME);
	//if (h == 0) error("gethostbyname");

	int port = (argc == 3)? (short)atoi(argv[2]) : SERVER_PORT;

	/* Allocate a socket (type SOCK_STREAM for TCP) */
	listening_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (listening_sd == -1) error("socket");

	int tr=1;
	// kill "Address already in use" error message
	if (setsockopt(listening_sd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	FD_SET(listening_sd,&readfds);
	if(listening_sd > fdMax)
		fdMax = listening_sd;

	/* Fill in '<struct sockaddr_in sin>' */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	/* Bind to an address */
	ret_val = bind(listening_sd, (struct sockaddr *)&sin, sizeof(sin));

	if (ret_val == -1) error("bind");

	/* listen */
	ret_val = listen(listening_sd, 10);
	if (ret_val == -1) error("listen");

	root = (struct client *)malloc( sizeof(struct client) );  
	root->next = NULL;   
	root->sd = listening_sd;
	// remaining values in struct are not initialised or used, this serves as
	// head of link list

	timeout.tv_sec = 60;	//1 minute
	timeout.tv_usec = 0;

	for(;;)
	{
		struct client *conductor;
		conductor = root;
		//Clearing & rebuilding the 'readfds' is a hack to get select working properly.
		FD_ZERO(&readfds);
		while (conductor != NULL)
		{
			FD_SET(conductor->sd,&readfds);
			conductor = conductor->next;
		}
		fflush(stdout);
		selectResponse = select(fdMax+1,&readfds,NULL,NULL,&timeout);
		if(selectResponse == 0)	{
			//Just to say Server is alive
			removeDeadSocket();
			timeout.tv_sec = 60;
			printf(".");
			continue;
		}
		conductor = root;
		while (conductor != NULL)
		{
			if (FD_ISSET(conductor->sd,&readfds)){
				serveRequest(conductor);
				break;
			}
			conductor = conductor->next;
		}
	}
	return 0;
}



