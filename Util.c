#include "Util.h"


#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

int start_winsock(void)
{
	WSADATA wsa;
	return WSAStartup(MAKEWORD(2, 0), &wsa);
}


long init_winsock() {

    long rc = start_winsock();

    if (rc != 0) {
        printf("Error: startWinsock, error code: %d \n", rc);
        return -1;
    }
    
    return rc;
}

SOCKET create_socket() {
    
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s == INVALID_SOCKET) {
        printf("Error: The socket could not be created, error code: %d \n", WSAGetLastError());
        return -1;
    }
   
    return s;
}

char* message_type_decode(UCHAR num_value) {

    switch (num_value) {
        case 0: 
            return "SA";
        case 1: 
            return "TRQ";
        case 2:
            return "ACK_TRQ";
        case 3:
            return "MSG_T";
        case 4:
            return "FILE_T";
        case 5:
            return "KA";
    }
}

/* https://people.cs.umu.se/isak/snippets/crc-16.c */
UINT16 get_crc(char* data_pointer, UINT16 length) {
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
        return (~crc);

    do
    {
        for (i = 0, data = (unsigned int)0xff & *data_pointer++;
            i < 8;
            i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else  crc >>= 1;
        }
    } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return (crc);
}

