#include "platform.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define MAX_CLIENTS 32

MUTEX clients_lock;
SOCKET clients[MAX_CLIENTS];
int client_count = 0;


THREAD_FUNC client_thread(void *arg) {
    SOCKET client = (SOCKET)(uintptr_t)arg;
    char buf[DEFAULT_BUFLEN];
    int len;
    while ((len = recv(client, buf, DEFAULT_BUFLEN, 0)) > 0) {
        buf[len] = '\0';

        MUTEX_LOCK(&clients_lock);
        for (int i = 0; i < client_count; ++i) {
            if (clients[i] != client) {
                send(clients[i], buf, len, 0);
            }
        }
        MUTEX_UNLOCK(&clients_lock);
    }
    
    MUTEX_LOCK(&clients_lock);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] == client) {
            clients[i] = clients[--client_count];
            break;
        }
    }
    MUTEX_UNLOCK(&clients_lock);
    CLOSE_SOCKET(client);
    THREAD_EXIT();
}


int main(void) {
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return EXIT_FAILURE;
    }
#endif
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    MUTEX_INIT(&clients_lock);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // need to dynamically change server ip
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        return 1;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        return 1;
    }

    
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        CLOSE_SOCKET(ListenSocket);
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        CLOSE_SOCKET(ListenSocket);
        return 1;
    }


    while (1) {
        // this to get maybe client ip??? tomorrow have to test it
        struct sockaddr_in client_ip;
        socklen_t len = sizeof(client_ip);
        SOCKET client = accept(ListenSocket, (struct sockaddr*)&client_ip, &len);
        char* ipstr = inet_ntoa(client_ip.sin_addr);
        printf("Client connected: %s:%d\n", ipstr, ntohs(client_ip.sin_port));

        if (client == INVALID_SOCKET) continue;
        MUTEX_LOCK(&clients_lock);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = client;
            thread_t tid;
            THREAD_CREATE(&tid, client_thread, (void*)(uintptr_t)client);
            THREAD_DETACH(tid);
        } else {
            CLOSE_SOCKET(client);
        }
        MUTEX_UNLOCK(&clients_lock);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}




/* TODO

sql database for users: username,sha256 password, admin true/false
dinamically change server ip
add TSL security layer



*/