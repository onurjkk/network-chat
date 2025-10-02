#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <process.h>
typedef HANDLE thread_t;
#define THREAD_FUNC unsigned __stdcall
#define THREAD_CREATE(thr, func, arg) *(thr) = (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL)
#define THREAD_EXIT() return 0
#define MUTEX CRITICAL_SECTION
#define MUTEX_INIT(m) InitializeCriticalSection(m)
#define MUTEX_LOCK(m) EnterCriticalSection(m)
#define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define MUTEX_DESTROY(m) DeleteCriticalSection(m)
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdint.h>
#include <netdb.h>         
#include <errno.h>         
#include <string.h>   
typedef pthread_t thread_t;
#define THREAD_FUNC void*
#define THREAD_CREATE(thr, func, arg) pthread_create(thr, NULL, func, arg)
#define THREAD_EXIT() return NULL
#define MUTEX pthread_mutex_t
#define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
#define MUTEX_LOCK(m) pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#define WSAGetLastError() (errno)   
#define ZeroMemory(Dst, Len) memset((Dst), 0, (Len))
typedef int SOCKET;           
#define INVALID_SOCKET -1     
#define SOCKET_ERROR   -1 
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUFLEN 256
#define DEFAULT_PORT "27015"
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
        // Broadcast to all clients
        MUTEX_LOCK(&clients_lock);
        for (int i = 0; i < client_count; ++i) {
            if (clients[i] != client) {
                send(clients[i], buf, len, 0);
            }
        }
        MUTEX_UNLOCK(&clients_lock);
    }
    // Remove client
    MUTEX_LOCK(&clients_lock);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] == client) {
            clients[i] = clients[--client_count];
            break;
        }
    }
    MUTEX_UNLOCK(&clients_lock);
#ifdef _WIN32
    closesocket(client);
#else
    close(client);
#endif
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
        #ifdef _WIN32
            closesocket(ListenSocket);
        #else
            close(ListenSocket);
        #endif
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        #ifdef _WIN32
            closesocket(ListenSocket);
        #else
            close(ListenSocket);
        #endif
        return 1;
    }


    while (1) {
        SOCKET client = accept(ListenSocket, NULL, NULL);
        if (client == INVALID_SOCKET) continue;
        MUTEX_LOCK(&clients_lock);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = client;
            thread_t tid;
            THREAD_CREATE(&tid, client_thread, (void*)(uintptr_t)client);
    #ifdef _WIN32
            CloseHandle(tid);
    #else
            pthread_detach(tid);
    #endif
        } else {
    #ifdef _WIN32
            closesocket(client);
    #else
            close(client);
    #endif
        }
        MUTEX_UNLOCK(&clients_lock);
    }

    WSACleanup();

    return 0;
}