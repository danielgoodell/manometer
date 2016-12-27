#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <drawing.h>
#include <time.h>
#include <pthread.h>
#include <manometer.h>


void *scanivalve(){

while(_fCloseThreads == 1){

	usleep(100000);
}
pthread_exit(NULL);

}
