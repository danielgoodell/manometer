#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<unistd.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
 
int main(int argc , char *argv[])
{
	 if(argc <2)
    {
        printf("Please provide a hostname to resolve.\n");
        exit(1);
    }
    
    printf("\n-------------------------------------------------------------------------------\n");
    
    int socket_desc;
    struct sockaddr_in server;
    struct addrinfo hints, *servinfo, *p;
    char message[100] = "scan\n"
	char server_reply[512];
    char *hostname = argv[1];
    char ip[100];
    int  rv;  	
  	char str[16];
    void *ptr;  
    ssize_t nread;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], "http", &hints, &servinfo)) != 0) {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    	exit(1);
	}

	// loop through all the results and connect to the first we can
	
	for(p = servinfo; p != NULL; p = p->ai_next) {
    	if ((socket_desc = socket(p->ai_family, p->ai_socktype,
       	 	p->ai_protocol)) == -1) {
       	 	perror("socket");
       	 	close(socket_desc);
       	 	continue;
    	}	

    	if (connect(socket_desc, p->ai_addr, p->ai_addrlen) == -1) {
        	perror("connect");
        	continue;
    	}

    	break; // if we get here, we must have connected successfully
    
	}

	if (p == NULL) {
    	// looped off the end of the list with no connection
    	fprintf(stderr, "failed to connect\n");
    	exit(2);
    }
    
    ptr = &((struct sockaddr_in *) p->ai_addr)->sin_addr;
	inet_ntop(AF_INET, ptr, str, 16);
    printf("Connected successfully to : %s (%s)\n", argv[1], str);

	freeaddrinfo(servinfo); // all done with this structure
   
    snprintf(message, 100, "GET / HTTP/1.1\r\nHost: %s \r\nConnection: close\r\n\r\n", argv[1]);

    if( send(socket_desc , &message , strlen(message) , 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
    puts("Requested default webpage. Response from Server:\n-------------------------------------------------------------------------------\n");
     
    //Receive a reply from the server
  
  	do{
  		nread = recv(socket_desc, server_reply, 512, 0);
  //	printf("\n\nsize read %d \n", nread); //size of block read from server
 		printf("%.*s", nread, server_reply);
 		if(strstr(server_reply, "\r\n0\r\n")!=NULL)
 		break;
 	} while(nread);
  
  	
  	printf("\n-------------------------------------------------------------------------------\n");
    close(socket_desc);
            
    return 0;
}
