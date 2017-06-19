#ifndef __SCANIVALVE_H__
#define __SCANIVALVE_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <drawing.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <termios.h>

void * scanivalve(void *);

void printdatarecord(void);

void copypress(void);

int set_interface_attribs(int fd, int speed);

#endif
