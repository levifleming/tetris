#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

// #include <arpa/inet.h>
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

#define TCP_CLIENT_BAD_SOCKET -1
#define TCP_CLIENT_DEFAULT_PORT "8081"
#define TCP_CLIENT_DEFAULT_HOST "localhost"

// Contains all of the information needed to create to connect to the server and send it a message.
typedef struct Config {
    char *port;
    char *host;
} Config;

int tcp_client_connect(Config config, SOCKET *ConnectSocket);

int tcp_client_send_request(SOCKET *ConnectSocket, char *message);

int tcp_client_receive_response(SOCKET *ConnectSocket, char *message);

void tcp_client_close(SOCKET ConnectSocket);

#endif