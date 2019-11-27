//////////////////////////////////////////////////////
// ----------- main.c -----------------
// Test application for RPi 4 UART to 485 data transfer
//
// By Papa Karlo Software, 2019
//
// ---------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>		
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART


#include <wiringPi.h>
#include <wiringSerial.h>

#include "crc16.h"

//Buffer for incoming physical-layer messages (PL-messages)
uint8_t		rx_buf[254];

//Index into the buffer
uint32_t	rx_i;



//------------------- transmit() -----------------
// Transmit data to UART (SRRP prot)
// fd		- serial port handle
// src_adr  - source data address
// size		- data size of bytes 
//------------------------------------------------
int transmit(int fd, uint8_t *src_adr, uint8_t size)
{
	uint32_t	length = size;
	uint32_t	length_to_write;
	uint32_t	written_length;
	uint8_t buf[256];
	uint16_t crc;
	uint32_t i, j;
	//Reset incoming message buffer
	rx_i = 0;
	memcpy(buf+1, src_adr, length);
	crc = crc16(src_adr, length);
	buf[length + 0] = (crc>>8)&0xFF;
	buf[length + 1] = crc&0xFF;
	buf[length + 2] = 0xFF;
	length_to_write = length + 4;
	//Replace 0xFFs
	i = 0;
	j = 1;
	length += 3;
	do { if (buf[j] == 0xFF) { buf[i] = j; i = j; } } while (++j <= length);

	for (int ii = 0; ii < length_to_write; ii++) {
		serialPutchar(fd, buf[ii]);
	}

	return 0;
}

//------------------- receive() -----------------
// Receive data from UART (SRRP prot)
// fd		- serial port handle
// dst_adr  - destination data address
// size		- data size of bytes address
//------------------------------------------------
int receive(int fd, uint8_t *dst_adr, uint8_t *size)
{
	uint8_t		portion_buf[1024 * 4];
	uint32_t	portion_length;
	uint8_t		c;
	uint32_t	i;

	//Initialize *length_adr
	*size = 0;

	extern int   serialDataAvail(const int fd);
	extern int   serialGetchar(const int fd);

	for (portion_length=0;; portion_length++) {
		if (serialDataAvail(fd)) {
			portion_buf[portion_length] = serialGetchar(fd);
		}
		else {
			break;
		}
	}
	
	if (!portion_length) return 0;
	//Parse the data portion
	if (rx_i + portion_length <= sizeof(rx_buf))
	{
		memcpy(rx_buf + rx_i, portion_buf, portion_length);
		rx_i += portion_length;
	}
	//Eliminate repeated delimiters, if any
	while (rx_buf[0] == 0xFF && rx_i > 0)
		memmove(rx_buf, rx_buf + 1, --rx_i);
	for (i = 0; i < rx_i && i < 254; i++)
		if (rx_buf[i] == 0xFF) //If the delimiter is found
		{
			rx_i = i;
			//Now 'rx_i' is length of the PL-message without delimiter; 253 max
			i = rx_buf[0];
			while (i < rx_i) //The loop will finish even on worst-case garbage
			{
				c = rx_buf[i];    //Save index
				rx_buf[i] = 0xFF; //Replace index with 0xFF
				i = c;
			}
			if (rx_i >= 4 && crc16_OK(rx_buf + 1, rx_i - 1))
				memcpy(dst_adr, rx_buf + 1, *size = rx_i - 3);
			rx_i = 0;
			return 0;
		}
	return 0;
}

int main()
{

	wiringPiSetupGpio();
	pinMode(19, OUTPUT);	// DE for 485 UART3
	pinMode(26, OUTPUT);	// DE for 485 UART5
	digitalWrite(19, LOW);  // Off
	digitalWrite(26, LOW);  // Off

	// Open serial UART3 & UART5
	// 8-N-1, 1000000 Hz
	int fd0 = serialOpen("//dev//ttyAMA1", 1000000);
	int fd1 = serialOpen("//dev//ttyAMA2", 1000000);

	for (int loop = 1;; loop++)
	{
		int	ii;
		uint8_t	data_in[256];
		uint8_t	data_out[256];

		serialFlush(fd0);
	
		int	ok_counter = 0;
		int	bad_counter = 0;

		for (loop=0;;loop++) {
			printf("LOOP %d: BAD=%d, OK=%d\n", loop,bad_counter, ok_counter);

			piLock(1);
			// write command "TEST" to slave addr 0
			data_out[0] = 0x0C;	// commant "TEST", address 0
			data_out[1] = 0x01;
			data_out[2] = 0x02;
			data_out[3] = 0x03;
			transmit(fd0, data_out, 4);
			piUnlock(1);

			for (ii = 0; ii < 100; ii++) {
				delayMicroseconds(1000);
				if (serialDataAvail(fd0)) break;
			}
			if (100 == ii) {
				printf("###ERROR: Can't get received data\n");
				bad_counter++;
				continue;
			}

			piLock(1);
			uint8_t	ret_size = 0;
			receive(fd0, data_in, &ret_size);
			for (ii = 0; ii < ret_size; ii++) {
				printf("RECV: %d  ", data_in[ii] );
			}
			piUnlock(1);
		}

	}
	serialClose(fd0);
	serialClose(fd1);

	return 0;
}




