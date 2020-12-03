#pragma once

int server_start();
UINT16 server_initialize_connection(long rc, SOCKET s, SOCKADDR_IN* remote_addr, int remote_addr_len, UINT32 *msg_type);