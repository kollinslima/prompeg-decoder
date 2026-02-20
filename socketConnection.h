#ifndef INCLUDED_SOCKET_H
#define INCLUDED_SOCKET_H

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SO_RCVBUF_SIZE 10485760
#define RECVBUFLEN 1500

typedef struct socket_utility_t {
    int media_Sockfd;
    int fecRow_Sockfd;
    int fecCol_Sockfd;
    int output_Sockfd;
    int fdmax;
    unsigned char *sockRecvBuf;
} socket_utility_t;

socket_utility_t* socket_utility_create(const char* mediaIP , const char* mediaPort, const char* outputIP , const char* outputPort);
void socket_utility_destroy(socket_utility_t *su);
bool socket_utility_is_multicast_address(const char* mediaIP);
int socket_utility_listen_socket(const char* mediaIP , const char* mediaPort , int recvBufSize);
int socket_utility_multicast_connect_socket(const char* multicastIP , const char* multicastPort);
int socket_utility_ucast_connect_socket(const char* unicastIP , const char* unicastPort);
int socket_utility_send_socket(const char* recvIP , const char* recvPort);
int socket_utility_maximum_of_three_num( int a , int b , int c );

#endif
