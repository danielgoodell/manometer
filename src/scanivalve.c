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

extern float comp_pres[25]; //compressor section pressures.
extern float sect_pres[39]; //test section pressures.

	
struct statisticaldatarecord{
		int32_t 	packet_type;
		int32_t 	packet_size;
		int32_t 	frame_number;
		int32_t 	scan_type;
		float 		frame_rate;
		int32_t 	valve_status;
		int32_t		units_index;
		float		units_conv_fact;
		int32_t 	ptp_scan_start_sec;
		int32_t 	ptp_scan_start_ns;
		uint32_t 	ext_trigger_time;
		float		temp[8];
		float		pressure[64];
		int32_t		frame_time_s;
		int32_t		frame_time_ns;
		int32_t		ext_trig_time_s;
		int32_t		ext_trig_time_ns;
		float		roll_ave_press[64];
		float		roll_max_press[64];
		float		roll_min_press[64];
		float		roll_rms_press[64];
		float		roll_stdev_press[64];
		float		roll_ave_excl_outlier[64];
		int32_t		filler[64];
	};

struct statisticaldatarecord * data;

void copypress(void){
	for(int j = 0; j<25; j++)
		comp_pres[j] = data->pressure[j];
	for(int j = 25; j<64; j++)
		sect_pres[j-25] = data->pressure[j];
}
	
void* scanivalve(void* arg)
{
	char str[INET_ADDRSTRLEN];
 	int i, nread;
	int socket_bin, socket_telnet;
	struct sockaddr_in p;
	char message[100] = "scan\n";
	char server_reply[2140];
	int port = 0;
	
	printf("Size of statistical data record: %lu", sizeof(*data));

	p.sin_addr.s_addr = inet_addr("191.30.90.110");
	p.sin_port = htons(port);
	p.sin_family = AF_INET;
 
	printf(" \nScanivalve address = %s : %d\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), ntohs(p.sin_port));

	socklen_t addr_size = sizeof(p);
	
	port = 23;
	p.sin_port=htons(port);

	if ((socket_telnet = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket for telnet server\n");
		goto end;
	}

	if ((i=connect(socket_telnet, (struct sockaddr*)&p, addr_size)) < 0) {
		printf("Could not connect to scanivalve telnet server @ %s:%d\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), ntohs(p.sin_port));
		goto end;
	}
		
	printf("Successfully connected to scanivalve telnet server\n");
	sleep(1);	
	
	port = 503;
	p.sin_port=htons(port);
	
	if ((socket_bin = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket for binary server\n");
		goto end;
	}

	if ((i=connect(socket_bin, (struct sockaddr*)&p, addr_size)) < 0) {
		printf("Could not connect to scanivalve binary server @ %s:%d\n\n", inet_ntop(AF_INET, &p.sin_addr, str, INET_ADDRSTRLEN), ntohs(p.sin_port) );
		goto end;
	}
	
	puts("Succesfully connected to binary server\n");

    snprintf(message, 100, "scan\r");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
    }

	puts("Binary Scan started - Waiting for response from Server:\n");
	     
    //Receive a reply from the server
  	do {
 		nread = recv(socket_bin, server_reply, 2140, 0);
		//printf("size read %d \n", nread); //size of block read from server
		if (nread == 2140){
			data = (struct statisticaldatarecord *) server_reply;
			copypress();			
		}
	} while(nread > 0 && _fCloseThreads);	
	
	while (_fCloseThreads) {

		nanosleep((const struct timespec[]){ { 0, 100000000L } }, NULL);
	}
	
		snprintf(message, 100, "stop\r");

    if(send(socket_telnet, &message, strlen(message) , 0) < 0)
    {
        puts("Send failed");
	}
	end:
	close(socket_telnet);
	puts("\nClosed telnet\n");
	close(socket_bin);
	puts("\nClosed binary\n");
	return NULL;
}

void printdatarecord(void){
	int i;

	printf("\nThe packet type is %d\n", data->packet_type);
	printf("The packet size is %d bytes\n", data->packet_size);
	printf("The frame number is %d\n", data->frame_number);
	printf("The scan type is %d (0-neg, 1-pos, 2-a/c)\n", data->scan_type);
	printf("The scanning rate is is %2.4f hz\n", data->frame_rate);
	printf("The valve status is %x (what does this mean?)\n", data->valve_status);
	printf("The units index is %d \n", data->units_index);	
	printf("The unit conversion factor %f \n", data->units_conv_fact);
	printf("The ptp scan start time is %d sec\n", data->units_index);
	printf("The ptp scan start time is %d ns \n", data->units_index);
	printf("The ext trigger time is %u usec \n", data->units_index);
	printf("The temperature array is:\n");
	for (i = 0; i<8; i++)
		printf("%2.2f ", data->temp[i]);
	printf("\nThe pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%2.2f ", data->pressure[i]);
	printf("\nThe frame time is: %d sec\n", data->frame_time_s);
	printf("\nThe frame time is: %d ns\n", data->frame_time_ns);
	printf("\nThe ext trigger time is: %d s\n", data->ext_trig_time_s);
	printf("\nThe ext trigger time is: %d ns\n", data->ext_trig_time_ns);
	
	printf("\nThe rolling average pressure array is:\n");
	for (i = 0; i<64; i++)
			printf("%2.2f ", data->roll_ave_press[i]);

	printf("\nThe rolling max pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%2.2f ", data->roll_max_press[i]);

	printf("\nThe rolling min pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%2.2f ", data->roll_min_press[i]);

	printf("\nThe rolling rms pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%2.2f ", data->roll_rms_press[i]);

	printf("\nThe rolling stdev of the pressure array is:\n");
	for (i = 0; i<64; i++)
		printf("%2.2f ", data->roll_stdev_press[i]);

	printf("\nThe rolling ave exc outliers is:\n");
	for (i = 0; i<64; i++)
		printf("%2.2f ", data->roll_ave_excl_outlier[i]);

	printf("\nWhat's in the filler? (hex):\n");
	for (i = 0; i<64; i++)
		printf("%X, ", data->filler[i]);

	printf("\nEnd of data record\n");
}

