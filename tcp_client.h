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

////////////////////////////////////////////////////
///////////// SOCKET RELATED FUNCTIONS /////////////
////////////////////////////////////////////////////

// Creates a TCP socket and connects it to the specified host and port. It returns the socket file
// descriptor.
int tcp_client_connect(Config config, SOCKET *ConnectSocket);

// Using the the action and message provided by the command line, format the data to follow the
// protocol and send it to the server. Return 1 (EXIT_FAILURE) if an error occurs, otherwise return
// 0 (EXIT_SUCCESS).
int tcp_client_send_request(SOCKET *ConnectSocket, char *message);

// Receive the response from the server. The caller must provide a function pointer that handles the
// response and returns a true value if all responses have been handled, otherwise it returns a
// false value. After the response is handled by the handle_response function pointer, the response
// data can be safely deleted. The string passed to the function pointer must be null terminated.
// Return 1 (EXIT_FAILURE) if an error occurs, otherwise return 0 (EXIT_SUCCESS).
int tcp_client_receive_response(SOCKET *ConnectSocket, char *message);

// Close the socket when your program is done running.
void tcp_client_close(SOCKET ConnectSocket);

#endif