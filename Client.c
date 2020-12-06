#include "Util.h"
#include "Client.h"


#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

UINT16 client_initialize_connection (long rc, SOCKET s, SOCKADDR_IN addr, SOCKADDR_IN* remote_addr, int remote_addr_len, UINT32 msg_size) {

    struct packet_header *header;
    char init[sizeof(struct packet_header)];

    header = (struct packet_header*) init;
  
    UINT16 fragment_size;
    boolean bad_fragment = true;
    
    while (bad_fragment) {

        printf("  Select fragment size (0 - %hu): ", MAX_FRAG_SIZE);
        scanf("%hu", &fragment_size);

        if (fragment_size < MAX_FRAG_SIZE && fragment_size > 0)
            bad_fragment = false;
        else
            printf("[-] Please select a valid fragment size.\n");
    }

    putchar('\n');


    // Keep alive method

    fd_set select_fds;
    struct timeval timeout;
    FD_ZERO(&select_fds);
    FD_SET(s, &select_fds);

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    while (1) {

        if (select(32, &select_fds, NULL, NULL, &timeout) == 0)                 
            break;

        else {

            rc = recvfrom(s, init, sizeof(struct packet_header), 0, (SOCKADDR*)remote_addr, &remote_addr_len);
            if (rc == SOCKET_ERROR) {
                printf("\n[-] Error: first recvfrom, error code: %d \n", WSAGetLastError());
                

            }
            else
            {
                printf("[+] Keep alive! Recieved [%s][%dB]\n", message_type_decode(header->message_type), rc);
            }
        
        }
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
        printf("\n[+] Initialization packet sent! [%s][%dB] \n",message_type_decode(header->message_type), rc);
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
        return 1;

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

        system("cls");
        printf("============================ Client =============================\n");
        printf("Target IP: %s\nTarget port: %d\n\n", ip, port);
        printf("Commands:\n");
        printf(" /e -flag [text] - simulates error on first packet send. flags: -c crc error, -t timeout error\n");
        printf(" /f - starts file transfer\n");
        printf(" /m - back to main menu\n");
        printf(" or just type in any message to send it on server!\n");


        


        char *msg = (char*)calloc(MAX_FILE_SIZE, sizeof(char));
        printf("\nEnter text:");
        gets();
        fgets(msg, MAX_TEXT_SIZE, stdin);
        
        // Retrieving message size and message type
        UINT32 msg_size = 0;
        UCHAR msg_type;

        char filename[256];

        if (msg[0] == '/' && msg[1] == 'm') {
            closesocket(s);
            WSACleanup();
            return 2;
        }


        msg_size = strlen(msg) - 1;
        msg = (char*)realloc(msg, msg_size * sizeof(char*));
        msg_type = 3;


        boolean crc_error = false, timeout_error = false;
        if (msg[0] == '/' && msg[1] == 'e') {
            msg_size = strlen(msg) - 7;
            msg = (char*)realloc(msg, msg_size * sizeof(char*));

            if (msg[2] == ' ' && msg[3] == '-') {

                if (msg[4] == 't')
                    timeout_error = true;

                else if (msg[4] == 'c')
                    crc_error = true;

                else {
                    printf("  Wrong syntax!\n");
                    printf("Press any key to continue...");
                    getch();
                    continue;
                }
            }
            else {
                printf("  Wrong syntax!\n");
                printf("Press any key to continue...");
                getch();
                continue;
            }
            msg += 6;
            msg_type = 3;
        }

        if (msg[0] == '/' && msg[1] == 'f') {

            printf("  Type the full path to the file to transfer: ");
            char path[30];
            scanf("%s", path);

            FILE* f = fopen(path, "rb");
            if (f == NULL) {
                printf("[-] Error: file not found!\n");
                printf("Press any key to continue...");
                getch();
                return 1;
            }

            // Calculating the size of the file 
            fseek(f, 0L, SEEK_END);
            msg_size = ftell(f);

            if (msg_size >= MAX_FILE_SIZE) {
                printf("[-] Error: file is too big.\n");
                printf("Press any key to continue...");
                getch();
                continue;
            }

            printf("  File size: %d B (%.2lf MB)\n", msg_size, (double)msg_size / MB);

            msg = (char*)realloc(msg, msg_size * sizeof(char*));

            // Back to the beggining of file
            rewind(f);

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
        unsigned int num_of_fragments = fragmentation(msg_size, fragment_size);

        // Set header
        data = (char*)malloc((sizeof(struct packet_header) + fragment_size) * sizeof(char));           
        header = (struct packet_header*)data;
        header->message_type = msg_type;
        header->fragment_size = fragment_size;


        // Start sending
        clock_t t;
        t = clock();
        int index = 0;

        for (int fragment = 0; fragment < num_of_fragments; fragment++) {

            header->seq_num = fragment;
           
            for (int i = 0; i < fragment_size; i++) {

                if (index == msg_size) {
                    header->fragment_size = i;
                    if (i == 0)
                    break;
                }

                data[sizeof(struct packet_header) + i] = msg[index++];

            }

            do {

                // Set header
                header->message_type = msg_type;
                header->crc = 0;

                if (crc_error) {
                    header->crc = get_crc(data, sizeof(struct packet_header) + header->fragment_size) + 1;
                    crc_error = false;
                }
                else
                    header->crc = get_crc(data, sizeof(struct packet_header) + header->fragment_size);

                // Send packet
                rc = sendto(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&addr, remote_addr_len);
                if (rc == SOCKET_ERROR) {
                    printf("Error: sendto, error code: %d \n", WSAGetLastError());
                    return 1;
                }
                if (header->message_type != 4)
                    printf("[+] Sent:     [MSG_TYPE %s] [SEQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

                // Recieve ack
                fd_set select_fds;                
                struct timeval timeout;           

                FD_ZERO(&select_fds);             
                FD_SET(s, &select_fds);           

                if (timeout_error) {
                    timeout.tv_sec = 0;		
                    timeout.tv_usec = 0;
                    timeout_error = false;
                }
                else {
                    timeout.tv_sec = 5;
                    timeout.tv_usec = 0;
                }

                if (select(32, &select_fds, NULL, NULL, &timeout) == 0)
                {
                    printf("[-] Error: timed out\n");
                    //header->message_type == 0;
                    fragment--;
                }
                else
                {
                    rc = recvfrom(s, data, sizeof(struct packet_header) + header->fragment_size, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
                    if (rc == SOCKET_ERROR)
                    {
                        printf("Error: recvfrom, error code: %d \n", WSAGetLastError());
                        return 1;
                    }
                    // Retrieve data from server                  
                    header = (struct packet_header*)data;

                    if (header->message_type != 4)
                        printf("[+] Recieved: [MSG_TYPE %s] [ARQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);
                }
                
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
        printf("\n[+] Sending took %lfs (%lfs per fragment)\n\n", time_taken, time_per_fragment);

        printf("Press any key to continue...");
        getch();
       
        header = NULL; 
    }
}

