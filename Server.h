#pragma once

int server_start();
void server_initialize_connection(long rc, SOCKET s, SOCKADDR_IN* remote_addr, int remote_addr_len);