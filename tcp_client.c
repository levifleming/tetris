#include "tcp_client.h"


static int request_count; // a running count of each request

/*
Description:
    This function connects the client (this program) to the server via tcp socket() function
Arguments:
    Config config: the config struct that was created from parsing the arguments
Return value:
    An int containing the socket file descriptor
*/

int tcp_client_connect(Config config, SOCKET *ConnectSocket) {
    struct addrinfo *res = NULL, *ptr = NULL, hints;
    //int sockfd;
    (*ConnectSocket) = INVALID_SOCKET;
    WSADATA wsadata;

    int iResult;
    iResult = WSAStartup(MAKEWORD(2,2), &wsadata);
    if(iResult != 0) {
        // fclose(fp);
        return 1;
    }

    // log_info("Connecting to %s:%s\r", config.host, config.port);
    //memset(&hints, 0, sizeof(hints));
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (iResult = getaddrinfo(config.host, config.port, &hints, &res) != 0) {
        // fputs("Couldn't get address info\r", fp);
        WSACleanup();
        // fclose(fp);
        return 1;
    }

    ptr = res;
    (*ConnectSocket) = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

    if ((*ConnectSocket) == INVALID_SOCKET) {
        // fputs("Couldn't connect to server\r\n", fp);
        //tcp_client_print_usage(!ERROR);
        freeaddrinfo(res);
        WSACleanup();
        // fclose(fp);
        return 1;
    }
    iResult = connect((*ConnectSocket), res->ai_addr, res->ai_addrlen);
    if(iResult == SOCKET_ERROR) {
        // fputs("Invalid socket\r\n", fp);
        closesocket((*ConnectSocket));
        (*ConnectSocket) = INVALID_SOCKET;
        // fclose(fp);
        return 1;
    } else {
        // fputs("Connected to server\r\n", fp);
        request_count = 0;
        freeaddrinfo(res);
    }
    // fputs("no errror", fp);
    // fclose(fp);
    return 0;
}

/*
Description:
    This function requests to send a message to the server via tcp socket send() function
Arguments:
    int sockfd: the socket file descriptor found from socket connection
    char *action: the string containing the action
    char *message: the string containing the message
Return value:
    Returns a 1 on failure, 0 on success
*/

int tcp_client_send_request(SOCKET *ConnectSocket, char *message) {
    // FILE *fp;

    // fp = fopen("client.txt", "w+");
    int length = strlen(message);
    int iResult;
    // char *sock_mes;
    // int mes_size = ((sizeof(char *) * strlen(message));
    // sock_mes = malloc(mes_size);

    // sprintf(sock_mes, "%d %s",length, message);
    // fputs(message, fp);
    int bytes_sent = send((*ConnectSocket), message, length, 0);
    // log_debug("Bytes sent: %d (%d/%d)\r", bytes_sent, bytes_sent, strlen(sock_mes));
    // free(sock_mes);
    request_count++;
    // log_debug("request_count: %d", request_count);
    // fputc('\n', fp);
    if (bytes_sent == -1) {
        // fputs("Bytes sent is incorrect\n", fp);
        closesocket((*ConnectSocket));
        WSACleanup();
        // fclose(fp);
        return EXIT_FAILURE;
    }
    // iResult = shutdown((*ConnectSocket), SD_SEND);
    // if (iResult == SOCKET_ERROR) {
    //     fputs("shutdown failed\n", fp);
    //     closesocket((*ConnectSocket));
    //     WSACleanup();
    //     fclose(fp);
    //     return EXIT_FAILURE;
    // }
    // fputs("send", fp);
    // fclose(fp);
    return EXIT_SUCCESS;
}

/*
Description:
    This function recieves a message from the server via tcp socket recv() function
Arguments:
    int sockfd: the socket file descriptor found from socket connection
    int (*handle_response)(char *): a function pointer to the handler of the response
Return value:
    Returns a 1 on failure, 0 on success
*/

int tcp_client_receive_response(SOCKET *ConnectSocket, char *message) {
    // for (int i = 0; i < request_count + 1; i++) {
    //     // log_debug("Recieving response header");
    //     int header_size = 0;
    //     char *response_header = malloc(sizeof(char));
    //     do {
    //         int current_bytes = recv(sockfd, (response_header + header_size), 1, 0);
    //         if (current_bytes == -1) {
    //             return EXIT_FAILURE;
    //         }
    //         header_size++;
    //         response_header = realloc(response_header, sizeof(char) * header_size);
    //     } while (response_header[header_size - 1] != ' ');
    //     response_header[header_size] = '\0';
    //     int response_length = atoi(response_header);
    //     free(response_header);
    //     // log_debug("Receiving response of length: %d", response_length);

    //     int bytes_received = 0;
    //     int old_bytes_received = bytes_received;
    //     char response[response_length];
    //     while (bytes_received < response_length) {
    //         bytes_received += recv(sockfd, response + bytes_received, response_length, 0);
    //         int recv_error = bytes_received - old_bytes_received;
    //         if (recv_error < 0) {
    //             return EXIT_FAILURE;
    //         }
    //         // log_debug("bytes recevied: %d/%d", bytes_received, response_length);
    //     }
    //     response[response_length] = '\0';
    //     if (handle_response(response)) {
    //         return EXIT_SUCCESS;
    //     }
    // }
    FILE *fp;
    fp = fopen("data/client.txt", "w+");
    
    int recvbuflen = 226;
    int iResult;
    char c[80];
    iResult = recv((*ConnectSocket), message, recvbuflen, 0);
    fputs(message, fp);
    if (iResult > 0){
        
    }
    else if (iResult == 0) {
        // fputs("Connection closed\n", fp);
    }
    else {
        
        fputs("recv failed\n", fp);
        closesocket(*ConnectSocket);
        fclose(fp);
        WSACleanup();
        return 1;
    }
        
    fputs("receive", fp);
    fclose(fp);
    return 0;
}

/*
Description:
    This function closes the client's connection to the server via tcp socket close() function
Arguments:
    int sockfd: the socket file descriptor found from socket connection
Return value:
    None
*/

void tcp_client_close(SOCKET ConnectSocket) {
    // shutdown the send half of the connection since no more data will be sent
    int iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
    }
    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    // log_info("Socket closed\n");
}