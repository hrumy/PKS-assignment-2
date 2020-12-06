#pragma once

int client_start();
UINT16 client_initialize_connection(long rc, SOCKET s, SOCKADDR_IN addr, SOCKADDR_IN* remote_addr, int remote_addr_len, UINT32 msg_size);
