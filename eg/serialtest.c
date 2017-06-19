#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS7;         /* 7-bit characters */
    tty.c_cflag |= PARENB;     /* parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int main()
{
    char *portname = "/dev/ttyUSB0";
    int fd;
    int wlen;

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 19200, 7 bits, even parity, 1 stop bit */
    set_interface_attribs(fd, B19200);

    /* simple output */
    wlen = write(fd, "#01PS\r\n", 7);
    if (wlen != 7) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd);    /* delay for output */

    /* simple noncanonical input */
	unsigned char buf[80]; 
	int rdlentot = 0;

	do {
        int rdlen;
		rdlen = read(fd, &buf[rdlentot], sizeof(buf) - (rdlentot+1));
        if (rdlen > 0){
			rdlentot = rdlentot + rdlen;
        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        }
        /* repeat read to get full message */
    } while (rdlentot < 13);

	buf[rdlentot] = '\0';

	int i = 0;
	for(i = 0; buf[i] != 6; i++){
		if(buf[i] >= '0' && buf[i] <= '9' || buf[i] == '.')
			buf[i-4] = buf[i];

	}
	buf[i-4] = '\0';
	float actualnumber = atof(buf);
	printf("String = %s\n", buf);
	printf("Float  = %f\n", actualnumber);
}
