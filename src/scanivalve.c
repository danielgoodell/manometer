#include <drawing.h>
#include <manometer.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

void* scanivalve(void* arg)
{
	int socket_desc;
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_desc == -1) {
		printf("Could not create socket");
	}

	while (_fCloseThreads == 1) {

		nanosleep((const struct timespec[]){ { 0, 100000000L } }, NULL);
	}
	return NULL;
}
