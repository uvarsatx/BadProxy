/* bad-proxy.c by Mason Hopkins and Kevin Charoenworawat */

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and getopt()*/
#include <math.h> 	/* for ceil() */
#include <time.h>   /* for struct timeval and RTT timer*/
#include <netdb.h>      /* for gethostbyname() */

#define MAXCONNECTIONS 1 /* Max client connection requests */
#define MAXBUFF 3000

typedef char* STRING;

typedef short Boolean;
#define TRUE 1
#define FALSE 0

int verbose, proxSock, clientSock, destSock;

void die(STRING s);
int scanargs(int argc, STRING *argv);
void runProxy(int clientSock);

/* "The proxy should wait for incoming HTTP trafﬁc on a user-speciﬁed port.
Your code will intercept the HTTP trafﬁc and determine the intended destination.
The proxy will then initiate its own TCP connection to the intended web server, 
retrieve the ﬁles and send them back to the client" */

int main(int argc, STRING *argv){
    unsigned short proxyPort;
    unsigned int clientLen;
    struct sockaddr_in proxyAddr, clientAddr;
    verbose = FALSE;

    proxyPort = scanargs(argc, argv);

	/* start proxy */
    if ((proxSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        die("Could not create TCP socket");
    }

    memset(&proxyAddr, 0, sizeof(proxyAddr));     /*zero out structure */
    proxyAddr.sin_family = AF_INET;                /* Internet address family */
    proxyAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    proxyAddr.sin_port = htons(proxyPort);              /* Local port */

    /* bind to local address */
    if (bind(proxSock, (struct sockaddr *) &proxyAddr, sizeof(proxyAddr)) < 0)
    {
        die("Could not bind to local address");
    }

    /* listen for connections */
    if(listen(proxSock, MAXCONNECTIONS) < 0)
    	die("listen() failed");

    while(1)
    {
    	/* size of in-out parameter */
    	clientLen = sizeof(clientAddr);

    	/* wait for client connection */
    	if((clientSock = accept(proxSock, (struct sockaddr *) &clientAddr, &clientLen)) < 0)
    		die("accept() failed");

    	runProxy(clientSock);
        close(clientSock);
    }
}

void die(STRING s){
    fprintf(stderr, "%s\n", s);
    exit(1);
}

int scanargs(int argc, STRING *argv){
    /* check each character of the option list for
       its meaning. */

    int x,p,v,port;

    if(argc != 5) // Check for correct number of arguments
	{
		fprintf(stderr, "Usage: %s -p <port> -v <verbose>\n", argv[0]);
		exit(1);
	}

    while((x = getopt(argc, argv, "p:v:")) != -1)
    {
    	switch(x)
    	{
    		case 'p':
    			port = atoi(strdup(optarg)); /* sets port from arg */
    			p = TRUE;
    			break;
    		case 'v':
    			if(atoi(strdup(optarg)) == 1) /* verbose 1 */
    				verbose = TRUE;
    			v = TRUE;
    			break;
    	}
    }

    if(!p || !v) /* command did not include either -p or -v */
    {
    	fprintf(stderr, "Usage: %s -p <port> -v <verbose>\n", argv[0]);
    	exit(1);
    }
    
    return port;
}


char* forwardRequest(char* get, char* host){
    struct hostent * dest;
    char * destIP;
    char * recBuff;
    recBuff = malloc(MAXBUFF);

    int destSock;
    struct sockaddr_in destAddr;
    unsigned short destPort = 80;
    int bytesRcvd = 0, totalBytesRcvd = 0;
    int buffLen = strlen(get);

    if((dest = gethostbyname(host)) == NULL)
        die("gethostbyname() failed");

    destIP = inet_ntoa(*((struct in_addr *)dest->h_addr));

    /* Create a reliable, stream socket using TCP */
    if ((destSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        die("handleRequest(): socket() failed");

    /* Construct the server address structure */
    memset(&destAddr, 0, sizeof(destAddr));     /* Zero out structure */
    destAddr.sin_family      = AF_INET;             /* Internet address family */
    destAddr.sin_addr.s_addr = inet_addr(destIP);   /* Server IP address */
    destAddr.sin_port        = htons(destPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(destSock, (struct sockaddr *) &destAddr, sizeof(destAddr)) < 0)
        die("handleRequest(): connect() failed");

    /* Send the string to the server */
    if (send(destSock, get, buffLen, 0) != buffLen)
        die("handleRequest(): send() sent a different number of bytes than expected");

    memset(recBuff,0,MAXBUFF);
    if ((bytesRcvd = recv(destSock, recBuff, MAXBUFF, 0)) <= 0)
         die("handleRequest(): recv() failed or connection closed prematurely");

     return recBuff;

}

void modResponse(char* buff){
    /* only need to worry about 200 Ok pages for verbose */

    Boolean pageWithGoogle = FALSE;
    Boolean foundGoogle = FALSE;
    Boolean pastHeader = FALSE;
    Boolean httpOK = FALSE;
    int i = 0;

    // Verify HTTP 200 OK
    if(buff[i+9] == '2')
        if(buff[i+10] == '0')
            if(buff[i+11] == '0')
                httpOK = TRUE;

    if(httpOK)
    {
        // adjust index until first '<' is found
        while(i < strlen(buff) && !pastHeader){
            if(buff[i] == '<')
                pastHeader = TRUE;

            i++;
        }

        while(i < strlen(buff)){
             if(buff[i] == 'G')  
                if(buff[i+1] == 'o')
                    if(buff[i+2] == 'o')
                        if(buff[i+3] == 'g')  
                           if(buff[i+4] == 'l')
                               if(buff[i+5] == 'e'){
                                    foundGoogle = TRUE;
                                    pageWithGoogle = TRUE;
                               }

            // Do the translation
            if(foundGoogle)
            {
                // Swap the first and last letter of google
                char temp = buff[i];
                buff[i] = buff[i+5];
                buff[i+5] = temp;

                // Swap the second and second to last letter of google
                temp = buff[i+1];
                buff[i+1] = buff[i+4];
                buff[i+4] = temp;

                // Swap the interior of google
                temp = buff[i+2];
                buff[i+2] = buff[i+3];
                buff[i+3] = temp;

                foundGoogle = FALSE; // reset boolean
                i+=5; // adjust index
            }

            i++;
        }

        if(verbose && pageWithGoogle)
            printf("Page found with \"Google\"\n");
    }
}

void runProxy(int clientSock){
    /* receive packets from client, establish new connection and relay them to their destination, 
    receive read/modify response packets, send them back to client? */

    char * recBuff = malloc(MAXBUFF);
    char * host;
    char * command;
    int msgSize = 1;                        /* received message size */
    Boolean g = FALSE, p = FALSE, h = FALSE;  /* used to determine if valid GET or POST call */

    /* forward and receive messages to web server */
    while(msgSize > 0)
    {
        /* zero out buffers */
        memset(recBuff, 0, MAXBUFF);

        /* receive get/post message from client */
        if((msgSize = recv(clientSock, recBuff, MAXBUFF, 0)) < 0)
            die("recv() getBuff failed");

        command = malloc(msgSize+1);
        memcpy(command, recBuff, msgSize-1);
        command[msgSize+1] = '\0';
        command[msgSize] = '\n';
        command[msgSize-1] = '\n';

        /* command validation */
        if(command[0] == 'G')
            if(command[1] == 'E')
                if(command[2] == 'T')
                    if(command[3] == ' ')
                    {
                        g = TRUE;
                    }
        if(command[0] == 'P')
            if(command[1] == 'O')
                if(command[2] == 'S')
                    if(command[3] == 'T')
                        if(command[4] == ' ')
                        {
                        p = TRUE;
                        }            

        /* Found either a good request for GET or POST */
        if(g || p){
            memset(recBuff, 0, MAXBUFF);

            /* receive host message from client */
            if((msgSize = recv(clientSock, recBuff, MAXBUFF, 0)) < 0)
                die("recv() hostBuff failed");

            if(recBuff[0] == 'H')
                if(recBuff[1] == 'o')
                    if(recBuff[2] == 's')
                        if(recBuff[3] == 't')
                            if(recBuff[4] == ':')
                                if(recBuff[5] == ' ')
                                {
                                    h = TRUE;
                                }  

            /* verified second line of request for "Host: " */
            if(h)
            {
                host = malloc(msgSize-8);
                memcpy(host, recBuff+6, msgSize-8);

                memset(recBuff, 0, MAXBUFF);

                /* verify complete request */
                if((msgSize = recv(clientSock, recBuff, MAXBUFF, 0)) < 0)
                    die("recv() recBuff failed");

                if(msgSize != 2)        /* received full message */
                {
                    die("Incorrect Telnet request");
                }
                
                /* if verbose, log GET and host to screen */
                if(verbose && g)
                {
                    fprintf(stderr, "%.*s at Host %s\n", (int)(strlen(command) - 3), command, host);
                }

                /* forward client request and receive destination response */ 
                memset(recBuff,0,MAXBUFF);
                recBuff = forwardRequest(command, host);

                /* scan and modify response from destination */
                modResponse(recBuff);

                /* send modified response back to client */
                if (send(clientSock, recBuff, MAXBUFF, 0) != MAXBUFF)
                    die("runProxy(): send() back to client failed");
            }
            else
                printf("Invalid Request. Waiting to handle next request.\n");

        } // if g || p bracket
        else
            printf("Invalid Request. Waiting to handle next request.\n");
        g = FALSE;
        p = FALSE;
        h = FALSE;
    } // while bracket

}
