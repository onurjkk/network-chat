#ifndef PLATFORM_H
#define PLATFORM_H

#define DEFAULT_BUFLEN 256
#define DEFAULT_PORT "27015"

#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")
typedef HANDLE thread_t;
#define THREAD_FUNC unsigned __stdcall
#define THREAD_CREATE(thr, func, arg) *(thr) = (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL)
#define THREAD_EXIT() return 0
#define THREAD_DETACH(tid) CloseHandle(tid)
#define MUTEX CRITICAL_SECTION
#define MUTEX_INIT(m) InitializeCriticalSection(m)
#define MUTEX_LOCK(m) EnterCriticalSection(m)
#define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define MUTEX_DESTROY(m) DeleteCriticalSection(m)
#define CLOSE_SOCKET(s) closesocket(s)
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
#define THREAD_DETACH(tid) pthread_detach(tid)
#define MUTEX pthread_mutex_t
#define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
#define MUTEX_LOCK(m) pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#define CLOSE_SOCKET(s) close(s)
#define WSAGetLastError() (errno)   
#define ZeroMemory(Dst, Len) memset((Dst), 0, (Len))
typedef int SOCKET;           
#define INVALID_SOCKET -1     
#define SOCKET_ERROR   -1 
#endif
#endif