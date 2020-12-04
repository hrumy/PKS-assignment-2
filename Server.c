
#include "Util.h"
#include "Server.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

UINT16 server_initialize_connection(long rc, SOCKET s, SOCKADDR_IN *remote_addr, int remote_addr_len, UINT32* msg_type) {

    struct packet_header* header;
    char init[sizeof(struct packet_header)];

    rc = recvfrom(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, &remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
        return 0;
    }
    else {
        header = (struct packet_header*)init;
        printf("[+] Initialization message received! Recieved [%s][%dB]\n", message_type_decode(header->message_type), rc);
    }

    UINT16 crc = header->crc;
    header->crc = 0;

    if (crc != get_crc(init, (UINT16*)sizeof(struct packet_header))) 
        header->message_type = 0;         
    else 
        header->message_type = 2;
    
    *msg_type = header->seq_num;

    rc = sendto(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: sendto, error code: %d \n", WSAGetLastError());
        return 0;
    }
    else {
        printf("[+] Acknowledgment of initialization sent! [%dB]\n", rc);
    }

    return header->fragment_size;
}
int server_start () {

    system("cls");
    printf("============================ Server =============================\n");

    // Socket creation
    long rc = init_winsock();
    SOCKET s = create_socket();
    if (rc == -1 || s == -1)
        return -1;

    // Conection
    SOCKADDR_IN addr;
    SOCKADDR_IN remote_addr;
    int remote_addr_len = sizeof(SOCKADDR_IN);
    
    char buf[10000];
    char buf2[300];

    // TODO Data header

    // Retrieving port
    short port;
    printf("Select port to sniff on: ");
    scanf("%hu", &port);

    // Setting port
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ADDR_ANY;

    // Binding to target pc
    rc = bind(s, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: bind, error code: %d \n", WSAGetLastError());
        return -1;
    }
    else {
        printf("Socket bound to port %d \n", port);
    }

    char *data;

    while (1) {

        // Initializing connection and retrieving max fragment size
        struct packet_header* header;

        UINT32 msg_size = 0;
        UINT16 fragment_size = server_initialize_connection(rc, s, &remote_addr, remote_addr_len, &msg_size);
        
        if (fragment_size == 0)
            return -1;

        data = (char*)malloc((sizeof(struct packet_header) + fragment_size) * sizeof(char));      
        header = (struct packet_header*)data;
        header->fragment_size = fragment_size;

        // Fragmentation
        unsigned int num_of_fragments;
        if (msg_size > fragment_size) {
            num_of_fragments = msg_size / fragment_size + 1;
        }
        else
            num_of_fragments = 1;



        char* msg = (char*)calloc(fragment_size, sizeof(char));
        int index = 0, error = 0;
        for (int i = 0; i < num_of_fragments; i++) {

            // Recieving packet 
            rc = recvfrom(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
            if (rc == SOCKET_ERROR)
            {
                printf("[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
                return 1;
            }
                       
            data = (char*)realloc(data, (sizeof(struct packet_header) + header->fragment_size) * sizeof(char));
            header = (struct packet_header*)data;

            printf("[+] Recieved: [MSG_TYPE %s] [SEQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

            int crc = header->crc;
            header->crc = 0;

            if (crc != get_crc(data, (sizeof(struct packet_header) + header->fragment_size))) {

                printf("[-] Error: bad packet.\n");

                // Send again
                header->message_type = 0;
                i--;

                // If packet was resend 5 times, connection will end
                if (error++ == 5) {
                    printf("[-] Fatal error: connection not safe.\n");
                    return 1;
                }
            }
            else {
                header->seq_num = i;
            }

            rc = sendto(s, data, (sizeof(struct packet_header) + header->fragment_size), 0, (SOCKADDR*)&remote_addr, remote_addr_len);
            if (rc == SOCKET_ERROR) {
                printf("Error: sendto, error code: %d \n", WSAGetLastError());
                return 1;
            }
            printf("[+] Sent: [MSG_TYPE %s] [SEQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

            
            if (header->message_type != 0) {
                for (int i = 0; i < header->fragment_size; i++) {
                    msg[index++] = data[sizeof(struct packet_header) + i];
                    msg[index] = '\0';
                }
            }
        }

        if (header->message_type == 3)
            printf("[+] Recieved message: %s\n", msg);
      
        header = NULL;
        /*
        sprintf(buf2, "You me too %s", buf);
        rc = sendto(s, buf2, strlen(buf2), 0, (SOCKADDR*)&remote_addr, remote_addr_len);
        if (rc == SOCKET_ERROR)
        {
            printf("Error: sendto, error code: %d \n", WSAGetLastError());
        }
        else {
            printf("%d bytes sent! \n", rc);
        }*/

    }
}