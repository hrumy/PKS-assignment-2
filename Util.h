#pragma once
#include <WinSock2.h>
#include <windows.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define MAX_FRAG_SIZE 1500

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