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
        return 1;
    }
    else 
        printf("Winsock started! \n");
    
    return rc;
}

SOCKET create_socket() {
    
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s == INVALID_SOCKET) {
        printf("Error: The socket could not be created, error code: %d \n", WSAGetLastError());
        return 1;
    }
    else
        printf("UDP socket created! \n");
   
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