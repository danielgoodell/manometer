#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <drawing.h>
#include <time.h>
#include <pthread.h>
#include <modbus.h>
#include <manometer.h>


void *modbus_server(){

while(_fCloseThreads == 1){
	usleep(100000);
}
pthread_exit(NULL);

}
