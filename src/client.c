#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


THREAD_FUNC recv_thread(void *arg) {
    SOCKET sock = (SOCKET)(uintptr_t)arg;
    char buf[DEFAULT_BUFLEN];
    int len;
    while ((len = recv(sock, buf, DEFAULT_BUFLEN, 0)) > 0) {
        buf[len] = '\0';
        printf("%s\n", buf);
        fflush(stdout);
    }
    printf("Disconnected from server.\n");
    exit(0);
    THREAD_EXIT();
}



int main(int argc, char **argv) {

#ifdef _WIN32
    WSADATA wsaData;
    int wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaerr != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", wsaerr);
        return EXIT_FAILURE;
    }
#endif
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;

    if (argc != 3) {
        printf("usage: %s <username> <server-ip>\n", argv[0]);
        return 1;
    }


    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    
    iResult = getaddrinfo(argv[2], DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        #ifdef _WIN32
            WSACleanup();
        #endif
        exit(EXIT_FAILURE);
    }

    
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            CLOSE_SOCKET(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        exit(EXIT_FAILURE);
    }


    const char *del = ": ";
    char sendbuf[DEFAULT_BUFLEN];
    char msg[DEFAULT_BUFLEN-strlen(del)-strlen(argv[1])];
    
    thread_t tid;
    THREAD_CREATE(&tid, recv_thread, (void*)(uintptr_t)ConnectSocket);
    while(fgets(msg, sizeof(msg), stdin)) {
        msg[strcspn(msg, "\n")] = 0;
        strcpy(sendbuf, argv[1]);
        strcat(sendbuf, del);
        strcat(sendbuf, msg);
        int sent = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
        if (sent == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            break;
        }
    }
       

    CLOSE_SOCKET(ConnectSocket);
#ifdef _WIN32
    WSACleanup();
#else
    pthread_detach(tid);
#endif
  
    return 0;
}