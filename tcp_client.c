#include "tcp_client.h"

int tcp_client_connect(Config config, SOCKET *ConnectSocket) {
    struct addrinfo *res = NULL, *ptr = NULL, hints;
    (*ConnectSocket) = INVALID_SOCKET;
    WSADATA wsadata;

    int iResult;
    iResult = WSAStartup(MAKEWORD(2,2), &wsadata);
    if(iResult != 0) {
        // fclose(fp);
        return 1;
    }

    //memset(&hints, 0, sizeof(hints));
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
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
        freeaddrinfo(res);
    }
    // fputs("no errror", fp);
    // fclose(fp);
    return 0;
}

int tcp_client_send_request(SOCKET *ConnectSocket, char *message) {
    FILE *fp;

    fp = fopen("data/client_send.txt", "w+");
    int length = strlen(message);
    int iResult;
    fputs(message, fp);
    int bytes_sent = send((*ConnectSocket), message, length, 0);
    if (bytes_sent == -1) {
        fputs("Bytes sent is incorrect\n", fp);
        closesocket((*ConnectSocket));
        WSACleanup();
        fclose(fp);
        return EXIT_FAILURE;
    }
    fputs("send", fp);
    fclose(fp);
    return EXIT_SUCCESS;
}

int tcp_client_receive_response(SOCKET *ConnectSocket, char *message) {
    FILE *fp;
    fp = fopen("data/client_receive.txt", "w+");
    
    int recvbuflen = 234;
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
}