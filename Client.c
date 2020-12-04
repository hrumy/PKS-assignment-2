#include "Util.h"
#include "Client.h"


#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

UINT16 client_initialize_connection(long rc, SOCKET s, SOCKADDR_IN addr, SOCKADDR_IN* remote_addr, int remote_addr_len, UINT16 msg_size) {

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
    header->seq_num = msg_size;
    header->crc = 0;
    header->crc = get_crc(init, (UINT16*)sizeof(struct packet_header));

    rc = sendto(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)&addr, remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("[-] Error: sendto, error code: %d \n", WSAGetLastError());
        return 0;
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
        return 0;
    }
    else
    {
        printf("\n[+] Acknowledment of initialization recieved! Recieved [%s][%dB]\n", message_type_decode(header->message_type), rc);
    }


    return fragment_size;
}

int client_start () {

    system("cls");
    printf("============================ Client =============================\n");

    // Socket creation
    long rc = init_winsock();
    SOCKET s = create_socket();
    if (rc == -1 || s == -1)
        return -1;

    // Connection 
    SOCKADDR_IN addr;
    SOCKADDR_IN remote_addr;
    int remote_addr_len = sizeof(SOCKADDR_IN);
    char ip[20];
    short port;

    // Data header
    struct packet_header *header;
    

    // Retrieving IP and port
    printf("Select recievers IP: ");
    scanf("%s", &ip);
    printf("Select recievers port: ");
    scanf("%hu", &port);

    // Setting IP and port
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    char *data;
   
    while (1)
    {      

        char *msg = (char*)calloc(MAX_TEXT_SIZE, sizeof(char));
        printf("Enter text:");
        gets();
        fgets(msg, MAX_TEXT_SIZE, stdin);
        
        // Retrieving message size and message type
        UINT16 msg_size;
        UCHAR msg_type;

        if (msg[0] != "/") {
            msg_size = strlen(msg) - 2;
            msg_type = 3;
        }


        // Initializing connection and setting fragment size
        UINT16 fragment_size = client_initialize_connection(rc, s, addr, &remote_addr, remote_addr_len, msg_size);
        if (fragment_size == 0) {
            return -1;
        }

        // Fragmentation
        unsigned int num_of_fragments;
        if (msg_size > fragment_size) {
            num_of_fragments = msg_size / fragment_size + 1;
        }
        else
            num_of_fragments = 1;


        // Set header
        data = (char*)malloc((sizeof(struct packet_header) + fragment_size) + 1 * sizeof(char));           
        header = (struct packet_header*)data;
        header->message_type = msg_type;
        header->fragment_size = fragment_size;
        

        
        // Start sending
        int index = 0;       
        for (int i = 0; i < num_of_fragments; i++) {

            header->seq_num = i;
           
            for (int j = 0; j < fragment_size; j++) {

                if (index == msg_size + 1) {
                    header->fragment_size = j;
                    break;
                }
                   
                data[sizeof(struct packet_header) + j] = msg[index++];
               
            }

            do {

                header->message_type = msg_type;
                header->crc = 0;
                header->crc = get_crc(data, sizeof(struct packet_header) + header->fragment_size) + 1;

                rc = sendto(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&addr, remote_addr_len);
                if (rc == SOCKET_ERROR) {
                    printf("Error: sendto, error code: %d \n", WSAGetLastError());
                    return 1;
                }           
                printf("[+] Sent: [MSG_TYPE %s] [SEQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

                rc = recvfrom(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
                if (rc == SOCKET_ERROR)
                {
                    printf("Error: recvfrom, error code: %d \n", WSAGetLastError());
                    return 1;
                }

                header = (struct packet_header*)data;
                printf("[+] Recieved: [MSG_TYPE %s] [ARQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

            } while (header->message_type == 0);

            
           
            
        }

        header = NULL;
            
            /*
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
            }*/
        
    }
}
