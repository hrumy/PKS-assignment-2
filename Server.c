
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
        printf("[+] Acknowledgment of initialization sent! [%dB]\n\n", rc);
    }

    //free(init);
    return header->fragment_size;
}
int server_start () {

    system("cls");
    printf("============================ Server =============================\n");

    // Socket creation
    long rc = init_winsock();
    SOCKET s = create_socket();
    if (rc == -1 || s == -1)
        return 1;

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
        printf("[-] Error: bind, error code: %d \n[-] Please select different port.\n", WSAGetLastError());
        return 1;
    }
    

    char *data;

    while (1) {

        system("cls");
        printf("======================= Server on port %d =======================\n", port);

        int option;
        printf("Commands:\n 1 - start sniffing\n 2 - back to menu\n\nEnter your option: ");
        scanf("%d", &option);

        if (option == 2)
            return 2;

        system("cls");
        printf("======================= Server on port %d =======================\n Sniffing..\n", port);

        // Initializing connection and retrieving max fragment size
        struct packet_header* header;

        UINT32 msg_size = 0;
        UINT16 fragment_size = server_initialize_connection(rc, s, &remote_addr, remote_addr_len, &msg_size);
        if (fragment_size == 0)
            return 1;


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

        
    
        // Start recieving
        clock_t t;
        t = clock();
        char* msg = (char*)calloc(msg_size, sizeof(char));
        int index = 0, error = 0;
        for (int fragment = 0; fragment < num_of_fragments; fragment++) {

            // Recieving packet 
            rc = recvfrom(s, data, sizeof(struct packet_header) + header->fragment_size + 1, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
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

            // Checking for error
            if (crc != get_crc(data, (sizeof(struct packet_header) + header->fragment_size))) {

                printf("[-] Error: bad packet.\n");

                // Send again
                header->message_type = 0;
                fragment--;

                // If packet was resend 5 times, connection will end
                if (error++ == 5) {
                    printf("[-] Fatal error: disconnecting, connection not safe.\n");
                    return 1;
                }
            }
            else {
                header->seq_num = fragment;
            }

            if (header->message_type != 0) {
                for (int i = 0; i < header->fragment_size; i++) {
                    msg[index++] = data[sizeof(struct packet_header) + i];
                    msg[index] = '\0';
                }
            }

            // Delete data after sizeof(header)
            char* buf = (char*)malloc(sizeof(struct packet_header) * sizeof(char));
            for (int i = 0; i < sizeof(struct packet_header); i++)
                buf[i] = data[i];

            strcpy(data, buf);

            rc = sendto(s, data, (sizeof(struct packet_header) /*+ header->fragment_size*/), 0, (SOCKADDR*)&remote_addr, remote_addr_len);
            if (rc == SOCKET_ERROR) {
                printf("Error: sendto, error code: %d \n", WSAGetLastError());
                return 1;
            }
            printf("[+] Sent:     [MSG_TYPE %s] [ARQ_NUM %d] [%dB]\n", message_type_decode(header->message_type), header->seq_num, rc);

            
            
        }

        // Message transfer
        if (header->message_type == 3)
            printf("\n[+] Recieved message: %s\n\n", msg);

        // File transfer
        if (header->message_type == 4) {

            // Recieving packet with filename
            rc = recvfrom(s, data, sizeof(struct packet_header) + 64, 0, (SOCKADDR*)&remote_addr, &remote_addr_len);
            if (rc == SOCKET_ERROR)
            {
                printf("[-] Error: recvfrom, error code: %d \n", WSAGetLastError());
                return 1;
            }
            data[rc] = '\0';

            char* filename = (char*)malloc(rc * sizeof(char));

            for (int i = 0; i < rc; i++)
                filename[i] = data[sizeof(struct packet_header) + i];

            int filename_len = strlen(filename);
            char* full_path = (char*)calloc((strlen(PATH) + filename_len), sizeof(char));
            strcpy(full_path, PATH);
            strcat(full_path, filename);


            // Saving file
            FILE* f = fopen(full_path, "wb");
            //fwrite(msg, msg_size, 1, f);
            for (int i = 0; i < msg_size; i++)
                fprintf(f, "%c", msg[i]);

            fclose(f);
            printf("\n[+] Recieved file: %s\n\n", full_path);
            //free(full_path);
        }
        
        t = clock() - t;
        double time_taken = ((double)t) / CLOCKS_PER_SEC;
        double time_per_fragment = time_taken / num_of_fragments;
        printf("[+] Recieving took %lfs (%lfs per fragment)\n\n", time_taken, time_per_fragment);

        printf("Press any key to continue...");
        getch();
        //free(data);
        header = NULL;
       

    }
}