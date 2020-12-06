#include "Util.h"
#include "Server.h"

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

unsigned int fragmentation(unsigned int msg_size, unsigned int fragment_size) {
    return (msg_size + fragment_size - 1) / fragment_size;
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
        for (i = 0, data = (unsigned int)0xff & *data_pointer++; i < 8; i++, data >>= 1)
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

DWORD WINAPI send_keepalive_thread(LPVOID lpParam) {

    struct keepalive_header* param;
    param = (struct keepalive_header*)lpParam;

    char keep_alive[sizeof(struct packet_header)];

    struct packet_header* header;
    header = (struct packet_header*)keep_alive;
    header->message_type = 5;
    header->fragment_size = 0;
    header->crc = 0;

    if (sendto(param->s, keep_alive, sizeof(struct packet_header), 0, (SOCKADDR*)param->remote_addr, sizeof(SOCKADDR_IN)) == -1) {
        printf("[-] Error: client disconnected, %d", WSAGetLastError());
    }

    header = NULL;
    param = NULL;

    return 0;

}

void send_keepalive(SOCKADDR_IN* remote_addr, SOCKET s) {

    DWORD dwThreadId;

    struct keepalive_header* Param;
    Param = (struct keepalive_header*)malloc(sizeof(struct keepalive_header));
    Param->remote_addr = remote_addr;
    Param->s = s;

    HANDLE hThread = CreateThread(NULL, 0, send_keepalive_thread, Param, 0, &dwThreadId);

    if (hThread == NULL)
        printf("[-] Error: CreateThread() failed, %d.\n", GetLastError());

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);


}
