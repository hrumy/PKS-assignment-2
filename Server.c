
#include "Util.h"
#include "Server.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

int server_initialize_connection(long rc, SOCKET s, SOCKADDR_IN *remote_addr, int remote_addr_len) {

    struct packet_header* header;
    char init[sizeof(struct packet_header)];

    rc = recvfrom(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, &remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
        return 1;
    }
    else {
        header = (struct packet_header*)init;
        printf("[+] Initialization message received! Recieved [%s][%dB]\n", message_type_decode(header->message_type), rc);
    }

    UINT16 crc = header->crc;
    header->crc = 0;

    if (crc != get_crc(init, (UINT16*)sizeof(struct packet_header))) {
        header->message_type = 0;      
    }
    else {
        header->message_type = 2;
    }
        
    rc = sendto(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: sendto, error code: %d \n", WSAGetLastError());
    }
    else {
        printf("[+] Acknowledgment of initialization sent! [%dB]\n", rc);
    }

    return header->fragment_size;
}
int server_start () {

    long rc = init_winsock();
    SOCKET s = create_socket();
    SOCKADDR_IN addr;
    SOCKADDR_IN remote_addr;
    int remote_addr_len = sizeof(SOCKADDR_IN);
    char buf[256];
    char buf2[300];

    short port;
    printf("Select port to sniff on: ");
    scanf("%hu", &port);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ADDR_ANY;

    rc = bind(s, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: bind, error code: %d \n", WSAGetLastError());
        return 1;
    }
    else {
        printf("Socket bound to port %d \n", port);
    }

    int fragment_size = server_initialize_connection(rc, s, &remote_addr, remote_addr_len);

    while (1) {

        rc = recvfrom(s, buf, 256, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);

        if (rc == SOCKET_ERROR)
        {
            printf("[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
            return 1;
        }
        else
        {
            printf("%d bytes received! \n", rc);
            buf[rc] = '\0';
        }
        printf("Received data: %s \n", buf);

        sprintf(buf2, "You me too %s", buf);
        rc = sendto(s, buf2, strlen(buf2), 0, (SOCKADDR*)&remote_addr, remote_addr_len);
        if (rc == SOCKET_ERROR)
        {
            printf("Error: sendto, error code: %d \n", WSAGetLastError());
        }
        else {
            printf("%d bytes sent! \n", rc);
        }

    }
}