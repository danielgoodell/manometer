#include <drawing.h>
#include <manometer.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>

void* scanivalve(void* arg)
{
	char str[INET_ADDRSTRLEN];
	printf("\nRunning scanivalve thread\n");
	int i;
	int socket_desc;
	struct sockaddr_in p;

	inet_pton(AF_INET, "192.90.30.110", &p.sin_addr);
	p.sin_port = 503;
	printf(" \nScanivalve address = %s:%d\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), p.sin_port);
	socklen_t addr_size = sizeof(p);

	if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket");
	}

	if ((i=connect(socket_desc, (struct sockaddr*)&p, addr_size)) < 0) {
		printf("Could not connect to host");
	}
	printf("\nConnect returned a %d\n", i);
	while (_fCloseThreads) {

		nanosleep((const struct timespec[]){ { 0, 100000000L } }, NULL);
	}
	return NULL;
}
