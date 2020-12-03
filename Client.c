#include "Util.h"
#include "Client.h"


#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

UINT16 client_initialize_connection(long rc, SOCKET s, SOCKADDR_IN addr, SOCKADDR_IN* remote_addr, int remote_addr_len) {

    struct packet_header *header;
    char init[sizeof(struct packet_header)];

    header = (struct packet_header*) init;
  
    UINT16 fragment_size;
    boolean bad_fragment = true;
    
    while (bad_fragment) {

        printf("Select fragment size (0 - %hu): ", MAX_FRAG_SIZE);
        scanf("%hu", &fragment_size);

        if (fragment_size < MAX_FRAG_SIZE && fragment_size > 0)
            bad_fragment = false;
        else
            printf("Please select a valid fragment size.\n");
    }
    header->fragment_size = fragment_size;
    header->message_type = 1;
    header->seq_num = 0;
    header->crc = 0;
    header->crc = get_crc(init, (UINT16*)sizeof(struct packet_header));

    rc = sendto(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)&addr, remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: sendto, error code: %d \n", WSAGetLastError());
        return 1;
    }
    else {
        printf("[+] Initialization packet sent! [%dB] \n Waiting for response", rc);
        for (int i = 0; i < 3; i++) {
            Sleep(1000);
            putchar('.');
        }
    }

    rc = recvfrom(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, &remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("\n[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
        return 1;
    }
    else
    {
        printf("\n[+] Acknowledment of initialization recieved! Recieved [%s][%dB]\n", message_type_decode(header->message_type), rc);
    }


    return fragment_size;
}

int client_start () {

    long rc = init_winsock();
    SOCKET s = create_socket();

    char buf[256];
    SOCKADDR_IN addr;
    SOCKADDR_IN remote_addr;
    int remote_addr_len = sizeof(SOCKADDR_IN);
    char ip[20];
    short port;
    struct packet_header *header;

    printf("Select recievers IP: ");
    scanf("%s", &ip);
    printf("Select recievers port: ");
    scanf("%hu", &port);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    //rc = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));

    client_initialize_connection(rc, s, addr, &remote_addr, remote_addr_len);

    while (1)
    {       
        printf("Enter text:");
        scanf("%s", buf);

        rc = sendto(s, buf, strlen(buf), 0, (SOCKADDR*)&addr, remote_addr_len);      
        if (rc == SOCKET_ERROR){
            printf("Error: sendto, error code: %d \n", WSAGetLastError());
            return 1;
        }
        else {
            printf("%d bytes sent! \n", rc);
        }

        rc = recvfrom(s, buf, 256, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
        if (rc == SOCKET_ERROR)
        {
            printf("Fehler: recvfrom, fehler code: %d\n", WSAGetLastError());
            return 1;
        }
        else
        {
            printf("%d bytes recieved!\n", rc);
            buf[rc] = '\0';
            printf("Recieved data: %s\n", buf);
        }
    }
}