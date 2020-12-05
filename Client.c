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
        printf("[+] Initialization packet sent! [%dB] \n", rc);
    }

    rc = recvfrom(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, &remote_addr_len);
    if (rc == SOCKET_ERROR) {
        printf("\n[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
        return 0;
    }
    else
    {
        printf("[+] Acknowledment of initialization recieved! Recieved [%s][%dB]\n\n", message_type_decode(header->message_type), rc);
    }
    if (header->message_type == 0) {
        printf("\n[-] Fatal error: bad connection.\n");
        return 0;
    }
        

    //free(init);
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

        char *msg = (char*)calloc(MAX_FILE_SIZE, sizeof(char));
        printf("Enter text:");
        gets();
        fgets(msg, MAX_TEXT_SIZE, stdin);
        
        // Retrieving message size and message type
        UINT32 msg_size = 0;
        UCHAR msg_type;

        char filename[256];

        if (msg[0] != '/') {
            msg_size = strlen(msg) - 1;
            msg_type = 3;
        }

        boolean error = false;
        if (msg[0] == '/' && msg[1] == 'e') {
            msg_size = strlen(msg) - 4;
            error = true;
            msg += 3;
            msg_type = 3;
        }

        if (msg[0] == '/' && msg[1] == 'f') {

            printf("  Type the full path to the file to transfer: ");
            char path[30];
            scanf("%s", path);

            FILE* f = fopen(path, "rb");
            if (f == NULL) {
                printf("[-] Error: file not found!\n");
                return 1;
            }

            // Calculating the size of the file 
            fseek(f, 0L, SEEK_END);
            msg_size = ftell(f);

            if (msg_size >= MAX_FILE_SIZE) {
                printf("[-] Error: file is too big.\n");
                continue;
            }

            printf("  File size: %dB\n", msg_size);

            msg = (char*)realloc(msg, msg_size * sizeof(char*));

            // Back to the beggining of file
            rewind(f);

            //fread(msg, msg_size, 1, f);
            for (int i = 0; i < msg_size; i++)
                fscanf(f, "%c", &msg[i]);


            fclose(f);

            printf("  Enter the name of file you wish to save it as (include .xxx): ");
            scanf("%s", filename);

            msg_type = 4;
        }



        // Initializing connection and setting fragment size
        UINT16 fragment_size = client_initialize_connection(rc, s, addr, &remote_addr, remote_addr_len, msg_size);
        if (fragment_size == 0) {
            return 1;
        }

        // Fragmentation
        unsigned int num_of_fragments;
        if (msg_size > fragment_size) {
            num_of_fragments = msg_size / fragment_size + 1;
        }
        else
            num_of_fragments = 1;


        // Set header
        data = (char*)malloc((sizeof(struct packet_header) + fragment_size) * sizeof(char));           
        header = (struct packet_header*)data;
        header->message_type = msg_type;
        header->fragment_size = fragment_size;

       
        
        // Start sending
        clock_t t;
        t = clock();
        int index = 0;
        boolean stop = false;
        for (int i = 0; i < num_of_fragments; i++) {

            header->seq_num = i;
           
            for (int j = 0; j < fragment_size; j++) {

                if (index == msg_size) {
                    header->fragment_size = j;
                    if (j == 0)
                        stop = true;
                    break;
                }

                data[sizeof(struct packet_header) + j] = msg[index++];
            }
            
           // if (stop)
           //     break;

            do {

                // Set header
                header->message_type = msg_type;
                header->crc = 0;

                if (error) {
                    header->crc = get_crc(data, sizeof(struct packet_header) + header->fragment_size) + 1;
                    error = false;
                }
                else
                    header->crc = get_crc(data, sizeof(struct packet_header) + header->fragment_size);

                // Send packet
                rc = sendto(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&addr, remote_addr_len);
                if (rc == SOCKET_ERROR) {
                    printf("Error: sendto, error code: %d \n", WSAGetLastError());
                    return 1;
                }           
                printf("[+] Sent: [MSG_TYPE %s] [SEQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

                // Recieve ack
                // TODO timeout
                rc = recvfrom(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
                if (rc == SOCKET_ERROR)
                {
                    printf("Error: recvfrom, error code: %d \n", WSAGetLastError());
                    return 1;
                }

                // Retrieve data from server
                header = (struct packet_header*)data;
                printf("[+] Recieved: [MSG_TYPE %s] [ARQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

            } while (header->message_type == 0);

        }

        if (msg_type == 4) {
            
            for (int i = 0; i < strlen(filename); i++)
                data[sizeof(struct packet_header) + i] = filename[i];


            rc = sendto(s, data, sizeof(struct packet_header) + strlen(filename), 0, (SOCKADDR*)&addr, remote_addr_len);
            if (rc == SOCKET_ERROR) {
                printf("Error: sendto, error code: %d \n", WSAGetLastError());
                return 1;
            }
        }

        t = clock() - t;
        double time_taken = ((double)t) / CLOCKS_PER_SEC;
        double time_per_fragment = time_taken / num_of_fragments;
        printf("[+] Sending took %lfs (%lfs per fragment)\n", time_taken, time_per_fragment);

        //free(data);
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
