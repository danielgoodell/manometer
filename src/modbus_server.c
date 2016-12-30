#include <drawing.h>
#include <manometer.h>
#include <modbus.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void* modbus_server(void* arg)
{

	while (_fCloseThreads == 1) {

		nanosleep((const struct timespec[]){ { 0, 100000000L } }, NULL);
	}
	return NULL;
}
