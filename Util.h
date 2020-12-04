#pragma once
#include <WinSock2.h>
#include <windows.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define MAX_FRAG_SIZE 1500
#define POLY 0x8408
#define MAX_TEXT_SIZE 2000

#pragma pack(1) 
struct packet_header {
	UCHAR message_type;
	UINT16 crc;
	UINT16 fragment_size;
	UINT32 seq_num;
};


int start_winsock(void);
int init_winsock();
SOCKET create_socket();
char* message_type_decode(UCHAR num_value);
UINT16 get_crc(char* data_pointer, UINT16 length);