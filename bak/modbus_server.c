#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <drawing.h>
#include <time.h>
#include <pthread.h>
#include <modbus.h>
#include <manometer.h>

void *modbus_server(void *arg){

while(_fCloseThreads == 1){

	nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
}
 return NULL;

}
