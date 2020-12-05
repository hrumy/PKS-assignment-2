#include <stdio.h>
#include "Util.h"
#include "Client.h"
#include "Server.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

int main()
{

    while (1) {

        system("cls");
        printf("============================= Menu ==============================\n\n");
        printf("Please select an option to continue:\n\n 1 - Client\n 2 - Server\n 3 - End\n\nEnter your option:  ");

        int option;
        scanf("%d", &option);

        if (option == 1)
            if (client_start() == 1) {
                printf("[-] Disconnected\nPress any key to continue...");
                getch();
            }



        if (option == 2)
            if (server_start() == 1) {
                printf("[-] Disconnected\nPress any key to continue...");
                getch();
            }
            

        if (option == 3)
            return 0;
             
    
    }
    return 0;
}
