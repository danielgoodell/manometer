#include <drawing.h>
#include <manometer.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h> 
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>

void* scanivalve(void* arg)
{
	char str[INET_ADDRSTRLEN];

	int i, nread;
	int socket_bin, socket_telnet;
	struct sockaddr_in p;
	char message[100] = "scan\n";
	char server_reply[2200];
	int port = 23;

	p.sin_addr.s_addr = inet_addr("191.30.90.110");
	p.sin_port = htons(port);
	p.sin_family = AF_INET;
 
	printf(" \nScanivalve address = %s : %d\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), ntohs(p.sin_port));

	socklen_t addr_size = sizeof(p);

	if ((socket_telnet = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket for telnet server\n");
		return NULL;
	}

	if ((i=connect(socket_telnet, (struct sockaddr*)&p, addr_size)) < 0) {
		printf("Could not connect to scanivalve telnet server @ %s:%d\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), ntohs(p.sin_port));
		return NULL;
	}
		
	printf("Successfully connected to scanvalve telnet server\n");

	port = 503;

	p.sin_port=htons(port);

	if ((socket_bin = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket for binary server\n");
	}

	if ((i=connect(socket_bin, (struct sockaddr*)&p, addr_size)) < 0) {
		printf("Could not connect to scanivalve binary server @ %s:%d\n\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), ntohs(p.sin_port) );
	}

  //	sleep(2);

	/*snprintf(message, 100, "\r\n");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
    }
*/
	snprintf(message, 100, "stop\r\n");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
	}
	
	snprintf(message, 100, "set format t c\r\n");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
    }
	
	snprintf(message, 100, "set format b s\r\n");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
    }

    snprintf(message, 100, "scan\r\n");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
    }

	//sleep(2);	

	puts("Scan Message sent - Response from Server:\n-------------------------------------------------------------------------------\n");
     
    //Receive a reply from the server
  
 	do{
  		nread = recv(socket_bin, server_reply, 2200, 0);
  	    printf("size read %d \n", nread); //size of block read from server
 		//printf("%.*s ", nread, server_reply);
		fwrite(server_reply, 1, sizeof(server_reply), stdout);
		if (_fCloseThreads != 1)
			return NULL;
 	} while(nread > 0);

	while (_fCloseThreads) {

		nanosleep((const struct timespec[]){ { 0, 100000000L } }, NULL);
	}
	
		snprintf(message, 100, "stop\r\n");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
	}
	close(socket_telnet);
	return NULL;
}
