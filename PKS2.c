#include <stdio.h>
#include "Util.h"
#include "Client.h"
#include "Server.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    printf("========================== Choose mode ==========================\n");
    printf("Please choose mode to continue (1 - client, 2 - server):");

    int option;
    scanf("%d", &option);

    if (option == 1) 
        client_start();
    
    

    if (option == 2)
        server_start();
             
    
    return 0;
}
