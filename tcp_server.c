#include "tcp_server.h"

int tcp_server_create(SOCKET *ListenSocket, char *port) {
    WSADATA wsaData;
    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        // fputs("WSAStartup failed\n", fp);
        // fclose(fp);
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    // char p[7];
    // sprintf(p, "%d", *port);
    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0) {
        // fputs("getaddrinfo failed\n", fp);
        WSACleanup();
        // fclose(fp);
        return 1;
    }

    *ListenSocket = INVALID_SOCKET;
    // Create a SOCKET for the server to listen for client connections
    
    *ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (*ListenSocket == INVALID_SOCKET) {
        // fputs("Invalid socket\n", fp);
        freeaddrinfo(result);
        WSACleanup();
        // fclose(fp);
        return 1;
    }
        // Setup the TCP listening socket
    iResult = bind( *ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        // fputs("bind failed\n", fp);
        freeaddrinfo(result);
        closesocket(*ListenSocket);
        WSACleanup();
        // fclose(fp);
        return 1;
    }
    freeaddrinfo(result);
    
    if ( listen( *ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        // fputs("Listen failed\n", fp);
        closesocket(*ListenSocket);
        WSACleanup();
        // fclose(fp);
        return 1;
    }
}


int tcp_server_accept_connection(SOCKET *ListenSocket, SOCKET *ClientSocket) {    
    //SOCKET ClientSocket;

    *ClientSocket = INVALID_SOCKET;

    // Accept a client socket
    // FILE *fp;
    // fp = fopen("server.txt", "w+");
    *ClientSocket = accept(*ListenSocket, NULL, NULL);
    if (*ClientSocket == INVALID_SOCKET) {
        // fputs("accept failed\n", fp);
        closesocket(*ListenSocket);
        WSACleanup();
        // fclose(fp);
        return 1;
    }
    // fputs("no error", fp);
    // fclose(fp);
    return 0; 
}

int tcp_server_receive_request(SOCKET *ClientSocket, char *message) {
    int recvbuflen = 226;
    int iResult;
    // FILE *fp;

    // fp = fopen("server.txt", "w+");
    // Receive until the peer shuts down the connection

    iResult = recv(*ClientSocket, message, recvbuflen, 0);
    if (iResult > 0) {
        // fputs(message, fp);
    } else if (iResult == 0) {
        // fputs("Connection closing...\n", fp);
    } else {
        char c[50];
        // sprintf(c, "recv failed: %d", WSAGetLastError());
        // fputs(c, fp);
        closesocket(*ClientSocket);
        WSACleanup();
        // fclose(fp);
        return 1;
    }
    // fputs("receive", fp);
    // fclose(fp);
    return 0;
}

int tcp_server_send_response(SOCKET *ClientSocket, char *message) {
    FILE *fp;

    fp = fopen("data/server.txt", "w+");
    fputs(message, fp);
    int iSendResult;
    int length = (sizeof(char *)) * strlen(message);
    // Echo the buffer back to the sender
    iSendResult = send(*ClientSocket, message, length, 0);
    if (iSendResult == SOCKET_ERROR) {
        fputs("send failed", fp);
        fclose(fp);
        closesocket(*ClientSocket);
        WSACleanup();
        return 1;
    }
    fputs("send", fp);
    fclose(fp);
    return 0;
    // printf("Bytes sent: %d\n", iSendResult);
}

void tcp_server_close(SOCKET ClientSocket, SOCKET ListenSocket) {
    // shutdown the send half of the connection since no more data will be sent
    int iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
    }
    // cleanup
    closesocket(ListenSocket);
    closesocket(ClientSocket);
    WSACleanup();
}


// int main(int argc, char *argv[]) { 
//     SOCKET ClientSocket;
//     tcp_server_connect(&ClientSocket);
//     char message[DEFAULT_BUFLEN];
//     tcp_server_receive_request(&ClientSocket, message);
//     tcp_server_send_response(&ClientSocket, "I hear you");
//     tcp_server_close(ClientSocket);

//     return 0;
// }

