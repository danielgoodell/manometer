#include <arpa/inet.h>
#include <drawing.h>
#include <errno.h>
#include <fcntl.h>
#include <manometer.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

extern float comp_pres[25]; //compressor section pressures.
extern float sect_pres[39]; //test section pressures.
extern float ref_press;

#define MESSAGE_SIZE 2200

struct statisticaldatarecord {
	int32_t packet_type;
	int32_t packet_size;
	int32_t frame_number;
	int32_t scan_type;
	float frame_rate;
	int32_t valve_status;
	int32_t units_index;
	float units_conv_fact;
	int32_t ptp_scan_start_sec;
	int32_t ptp_scan_start_ns;
	uint32_t ext_trigger_time;
	float temp[8];
	float pressure[64];
	int32_t frame_time_s;
	int32_t frame_time_ns;
	int32_t ext_trig_time_s;
	int32_t ext_trig_time_ns;
	float roll_ave_press[64];
	float roll_max_press[64];
	float roll_min_press[64];
	float roll_rms_press[64];
	float roll_stdev_press[64];
	float roll_ave_excl_outlier[64];
	int32_t filler[64];
};

struct statisticaldatarecord* data;

void copypress(void)
{

	//Pull pressures from data record and write them to test section and compressor pressure arrays.

	for (int j = 0; j < 29; j++)
		sect_pres[j] = data->pressure[j] + ref_press;

	for (int j = 60; j < 64; j++)
		comp_pres[j - 60] = data->pressure[j] + ref_press;
}

int connectScanivalve(struct sockaddr_in addr)
{
	int sock;
	int status;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	int wait_time = 0;
	struct timespec t;

	clock_gettime(CLOCK_BOOTTIME, &t);
	//	printf("%llu Seconds since power on. ",(unsigned long long)t.tv_sec);
	wait_time = 90 - t.tv_sec;
	if (wait_time > 0) {
		printf("There hasn't been enough time since reboot to connect to the scanivalve. \n");
		printf("Scanivalve takes over two minutes to bootup. Waiting %d seconds.\n", wait_time);
		sleep(wait_time);
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket for telnet server\nERROR: %s\n", strerror(errno));
	}

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	fcntl(sock, F_SETFL, O_NONBLOCK);

	if ((status = connect(sock, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
		if (errno != EINPROGRESS)
			return status;
	}

	status = select(sock + 1, NULL, &set, NULL, &timeout);

	fcntl(sock, F_SETFL, (fcntl(sock, F_GETFL, 0) & ~O_NONBLOCK));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	if (status > 0)
		return sock; //successfully connected
	else
		return -1;
}

int connectHeise(void)
{
	int fd;
	char* portname = "/dev/ttyUSB0";

	//Open the serial port to communicate with the Heise DXD.
	if ((fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC)) < 0) {
		if (errno != EINPROGRESS)
			return fd;
	}

	set_interface_attribs(fd, B19200); 	//baudrate 19200, rest of attributes are in the set_interface _attribs
	if (fd > 0)
		return fd; //successfully connected
	else
		return -1;
}

int set_interface_attribs(int fd, int speed)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
		printf("Error from tcgetattr: %s\n", strerror(errno));
		return -1;
	}

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);

	tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS7;		/* 7-bit characters */
	tty.c_cflag |= PARENB;  /* parity bit */
	tty.c_cflag &= ~CSTOPB; /* only need 1 stop bit */
	//   tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	/* set up for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;
	/* 	fetch bytes as they become available 
	*	If response isn't received within 0.5 
	*	secs, reads will return a zero.
	*/
	tty.c_cc[VMIN] = (unsigned char)0;
	tty.c_cc[VTIME] = (unsigned char)4;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		printf("Error from tcsetattr: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
// readHeise: read the next string received from the Heise DXD after send.
//recieved up to bufferlen, and received until it gets ascii 0x06.
int readHeise(int fd, char buf[], int bufferlen)
{
	int rdlentot = 0;
	bool end = false;
	int rdlen = 0;
	memset(buf, 0, bufferlen); // erase last number from the buffer.
	do {
		rdlen = 0;
		if ((rdlen = read(fd, &buf[rdlentot], bufferlen - (rdlentot + 1))) > 0) {
			for (int i = rdlentot; (i <= rdlentot + rdlen); i++) {
				if (buf[i] == 0x6) {
					end = true;
					break;
				}
			}
			rdlentot = rdlentot + rdlen;
		} else if (rdlen <= 0) {

			return -1;
		}
	} while (end == false);
	buf[rdlentot + rdlen] = '\0';
	return 1;
}

void* scanivalve(void* arg)
{
	char str[INET_ADDRSTRLEN];
	int nread;
	int socket_bin, socket_telnet;
	struct sockaddr_in addr;
	char message[100];
	char server_reply[2200];
	char buf[20]; // Buffer to store the Heise DXD response
	int fd;		  // Serial Port file descriptor
	int wlen, rlen;

	addr.sin_addr.s_addr = inet_addr("191.30.90.110");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(23);

	printf(" \nScanivalve pressure scanner ip address: %s\n", inet_ntop(AF_INET, &addr.sin_addr, str, INET_ADDRSTRLEN));

	if ((socket_telnet = connectScanivalve(addr)) < 0) { //Connect to telnet server
		printf("Failed to connect to Scanivalve telnet server, shutting down.\n");
		close(socket_telnet);
		_fCloseThreads = 0;
		return NULL;
	}

	addr.sin_port = htons(503);

	if ((socket_bin = connectScanivalve(addr)) < 0) { //Connect to binary server
		printf("Failed to connect to Scanivalve binary server, shutting down.\n");
		close(socket_telnet);
		close(socket_bin);
		_fCloseThreads = 0;
		return NULL;
	}
	if ((fd = connectHeise()) < 0) { //Open serial Port for Heise DXD
		printf("Failed to open serial port\n");
		close(socket_telnet);
		close(socket_bin);
		_fCloseThreads = 0;
		return NULL;
	}

	nanosleep((const struct timespec[]){ { 0, 500000000L } }, NULL);
//	tcflush(fd, TCIFLUSH); //Flush reads and writes if there was a previous error connecting.

	/* Communicate with the Heise DXD to verify proper communication and determine address */

	if ((wlen = write(fd, "#**AD\r", 6)) != 6) {
		printf("Error from write: %d, %s\n", wlen, strerror(errno));
		printf("Failed to send command to the serial port. The serial adapter may not be functioning properly.");
		close(socket_telnet);
		close(socket_bin);
		_fCloseThreads = 0;
		return NULL;
	}
	printf("Requested address of Heise DXD\n");

	tcdrain(fd); //Wait until the previous write command has completed before listening for response.

	// Receive the response from the Heise DXD to verify operation and programmed address.

	if ((rlen = readHeise(fd, buf, sizeof(buf))) < 0) {
		printf("Heise DXD Pressure transducer failed to respond\n");
		printf("It may be disconnected or broken. Exiting program...\n");
		close(fd);
		close(socket_telnet);
		close(socket_bin);
		_fCloseThreads = 0;
		return NULL;
	}

	printf("\nReported address of the Heise DXD is %.*s\n", 5, buf);

	snprintf(message, 100, "scan\r\n");

	if (send(socket_telnet, &message, strlen(message), 0) < 0) {
		printf("Scan start command failed to be sent after connecting.\n");
		printf("Error given: %s\n", strerror(errno));
		printf("Scanivalve may have crashed or may need just need more time to complete bootup.\n");
		close(fd);
		close(socket_telnet);
		close(socket_bin);
		_fCloseThreads = 0;
		return NULL;
	}

	printf("Succesfully connected to the Scanivalve pressure scanner. Receiving data.\n");

	//Receive a reply from the server
	char* server_reply_p;
	do {
		nread = 0;
		server_reply_p = server_reply;
		while (nread < 2140 && nread >= 0 && _fCloseThreads) {
			nread = nread + recv(socket_bin, server_reply_p, (MESSAGE_SIZE - nread), 0);
			if (((int32_t)*server_reply) != 11) {
				nread = 0;
				server_reply_p = server_reply;
				continue;
			}
			if (nread == -1 && errno == EAGAIN) {
				nread = 0;
				continue;
			}
			if (nread < 2140 && nread > 0)
				server_reply_p = &server_reply[nread];
		}
		data = (struct statisticaldatarecord*)server_reply;
		copypress();
		printdatarecord();

		//send pressure read command to the Heise.
		wlen = write(fd, "#01PS\r", 6);
		if (wlen != 6) {
			printf("Error from write: %d, %d\n", wlen, errno);
		}
		tcdrain(fd); /* delay for output */

		if ((rlen = readHeise(fd, buf, sizeof(buf))) < 0) {
			printf("Heise DXD Pressure transducer failed to respond\n");
			printf("It may be disconnected or broken. Exiting program...\n");
			close(fd);
			close(socket_telnet);
			close(socket_bin);
			_fCloseThreads = 0;
			return NULL;
		}

		//Remove everything that isn't the pressure from the ascii response from the DXD.
		int i, j = 0;
		for (i = 0; buf[i] != 6; i++) {
			if ((buf[i] >= '0' && buf[i] <= '9') || buf[i] == '.' || i == sizeof(buf) - 1) {
				if (j == 0) {
					j = i;
				}
				buf[i - j] = buf[i];
			}
		}
		buf[i - j] = '\0';
		ref_press = atof(buf);
		//Continue as long as we are receiving from the Scanivalve or a shutdown is initiated.
	} while (nread > 0 && _fCloseThreads);

	if (nread < 0)
		printf("Stopping due to error: %s\n", strerror(errno));

	snprintf(message, 100, "stop\r\n");

	if (send(socket_telnet, &message, strlen(message), 0) < 0)
		puts("Scan stop command failed to be sent");
	close(fd);
	close(socket_telnet);
	close(socket_bin);
	_fCloseThreads = 0;
	return NULL;
}

void printdatarecord(void)
{
	int i;

	printf("\033[H  ");
	printf("\033[2J \n");
	printf("The ref pressure is %f\n", ref_press);
	printf("The packet type is %d\n", data->packet_type);
	printf("The packet size is %d bytes\n", data->packet_size);
	printf("The frame number is %d\n", data->frame_number);
	//	printf("The scan type is %d (0-neg, 1-pos, 2-a/c)\n", data->scan_type);
	printf("The scanning rate is is %2.4f hz\n", data->frame_rate);
	//	printf("The valve status is %x (what does this mean?)\n", data->valve_status);
	//	printf("The units index is %d \n", data->units_index);
	//	printf("The unit conversion factor %f \n", data->units_conv_fact);
	//	printf("The ptp scan start time is %d sec\n", data->units_index);
	//	printf("The ptp scan start time is %d ns \n", data->units_index);
	//	printf("The ext trigger time is %u usec \n", data->units_index);
	printf("The temperature array is:\n");
	for (i = 0; i < 8; i++)
		printf("%2.2f\t", data->temp[i]);
	printf("\nThe pressure array is:\n");
	for (i = 0; i < 64; i++)
		printf("%c%2.2f%c", (data->pressure[i] >= 0) ? ' ' : '-', fabs(data->pressure[i]), (i % 16 == 15 || i == 63) ? '\n' : '\t');
	printf("\n10x10 Supersonic Wind Tunnel Manometer - NASA Glenn Research Center - 2017\n");
	printf("\n(Designed and built at 8x6/9x15 SWT/LSWT - NASA Glenn Research Center - 2017)\n");
	//	printf("\nThe frame time is: %d sec\n", data->frame_time_s);
	//	printf("\nThe frame time is: %d ns\n", data->frame_time_ns);
	//	printf("\nThe ext trigger time is: %d s\n", data->ext_trig_time_s);
	//	printf("\nThe ext trigger time is: %d ns\n", data->ext_trig_time_ns);
	/*	
	printf("\nThe rolling average pressure array is:\n");
	for (i = 0; i<64; i++)
			printf("%c%2.2f%c", (data->roll_ave_press[i] >= 0) ? ' ' : '-', fabs(data->roll_ave_press[i]), (i%16==15 || i==63) ? '\n' : '\t');

	printf("\nThe rolling max pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%c%2.2f%c", (data->roll_max_press[i] >= 0) ? ' ' : '-', fabs(data->roll_max_press[i]), (i%16==15 || i==63) ? '\n' : '\t');

	printf("\nThe rolling min pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%c%2.2f%c", (data->roll_min_press[i] >= 0) ? ' ' : '-', fabs(data->roll_min_press[i]), (i%16==15 || i==63) ? '\n' : '\t');

//	printf("\nThe rolling rms pressure array is:\n");
//	for (i = 0; i<64; i++)
//		printf("%c%2.2f%c", (data->roll_rms_press[i] = 0) ? ' ' : '-', fabs(data->roll_rms_press[i]), (i%16==15 || i==63) ? '\n' : '\t');

	printf("\nThe rolling stdev of the pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%c%2.2f%c", (data->roll_stdev_press[i] >= 0) ? ' ' : '-', fabs(data->roll_stdev_press[i]), (i%16==15 || i==63) ? '\n' : '\t');

	printf("\nThe rolling ave exc outliers is:\n");
	for (i = 0; i<64; i++)
		printf("%c%2.2f%c", (data->roll_ave_excl_outlier[i] >= 0) ? ' ' : '-', fabs(data->roll_ave_excl_outlier[i]), (i%16==15 || i==63) ? '\n' : '\t');
*/
	//	printf("\nWhat's in the filler? (hex):\n");
	//	for (i = 0; i<64; i++)
	//		printf("%X, ", data->filler[i]);
	printf("\n");
}
