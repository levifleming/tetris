#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#define DEFAULT_PORT "8088"
#define DEFAULT_BUFLEN 512

#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>
#include <getopt.h>
// #include <netdb.h>
// #include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int tcp_server_create(SOCKET *ListenSocket, char *port);

int tcp_server_accept_connection(SOCKET *ListenSocket, SOCKET *ClientSocket);

int tcp_server_receive_request(SOCKET *ClientSocket, char *message);

int tcp_server_send_response(SOCKET *ClientSocket, char *message);

void tcp_server_close(SOCKET ClientSocket, SOCKET ListenSocket);
#endif