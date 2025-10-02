#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
typedef HANDLE thread_t;
#define THREAD_FUNC unsigned __stdcall
#define THREAD_CREATE(thr, func, arg) *(thr) = (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL)
#define THREAD_EXIT() return 0
#pragma comment(lib, "Ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdint.h>
typedef pthread_t thread_t;
#define THREAD_FUNC void*
#define THREAD_CREATE(thr, func, arg) pthread_create(thr, NULL, func, arg)
#define THREAD_EXIT() return NULL
typedef int SOCKET;           // <-- add this for SOCKET
#define INVALID_SOCKET -1     // <-- add this for INVALID_SOCKET
#define SOCKET_ERROR   -1 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define DEFAULT_BUFLEN 256
#define DEFAULT_PORT "27015"
#define MAX_CLIENTS 32

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



int __cdecl main(int argc, char **argv) {

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
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Validate the parameters
    if (argc != 3) {
        printf("usage: %s <username> <server-ip>\n", argv[0]);
        return 1;
    }


    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[2], DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        #ifdef _WIN32
            WSACleanup();
        #endif
        exit(EXIT_FAILURE);
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            #ifdef _WIN32
                closesocket(ConnectSocket);
            #else
                close(ConnectSocket);
            #endif
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
            printf("send failed with error: %ld\n", WSAGetLastError());
            break;
        }
    }
       

#ifdef _WIN32
    closesocket(ConnectSocket);
    WSACleanup();
#else
    close(ConnectSocket);
    pthread_detach(tid);
#endif
  
    return 0;
}