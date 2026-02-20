#include "socketConnection.h"

bool socket_utility_is_multicast_address(const char* mediaIP) {
    char temp[4];
    int i = 0;
    while(mediaIP[i] != '.' && i < 3 && mediaIP[i] != '\0') {
        temp[i] = mediaIP[i];
        i++;
    }
    temp[i] = '\0';
    int firstByteVal = atoi(temp);

    if(firstByteVal >= 224 && firstByteVal <= 239) {
        return true;
    }
    else {
        return false;
    }
}

int socket_utility_listen_socket(const char* mediaIP , const char* mediaPort , int recvBufSize) {
    int sockfd;
    struct addrinfo hints = { 0 };
    struct addrinfo* localAddr = 0;
    struct addrinfo* mediaAddr = 0;
    int yes = 1;

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_NUMERICHOST;

    int addrInfoStatus;
    if((addrInfoStatus = getaddrinfo(mediaIP , NULL , &hints , &mediaAddr)) != 0) {
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    hints.ai_family = mediaAddr->ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if((addrInfoStatus = getaddrinfo(NULL , mediaPort , &hints , &localAddr)) != 0) {
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    if((sockfd = socket(localAddr->ai_family , localAddr->ai_socktype , 0)) < 0) {
        perror("socket() failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        return -1;
    }

    if(setsockopt(sockfd , SOL_SOCKET , SO_REUSEADDR , (char*)&yes , sizeof(int)) == -1) {
        perror("setsockopt() failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        return -1;
    }

    if(bind(sockfd , localAddr->ai_addr , localAddr->ai_addrlen) != 0) {
        perror("bind() failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        return -1;
    }

    int outputValue = 0;
    socklen_t outputValue_len = sizeof(outputValue);
    int defaultBuf;

    if(getsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , &outputValue_len) != 0) {
        perror("getsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        return -1;
    }
    defaultBuf = outputValue;

    outputValue = recvBufSize;
    if(setsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , sizeof(outputValue)) != 0) {
        perror("setsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        return -1;
    }
    if(getsockopt(sockfd , SOL_SOCKET , SO_RCVBUF , (char*)&outputValue , &outputValue_len) != 0) {
        perror("getsockopt failed");
        if(localAddr)
            freeaddrinfo(localAddr);
        if(mediaAddr)
            freeaddrinfo(mediaAddr);
        return -1;
    }

    printf("tried to set socket receive buffer from %d to %d, got %d\n", defaultBuf , recvBufSize , outputValue);

    if(socket_utility_is_multicast_address(mediaIP)) {
        struct ip_mreq multicastRequest;
        memcpy(&multicastRequest.imr_multiaddr , &((struct sockaddr_in*)(mediaAddr->ai_addr))->sin_addr , sizeof(multicastRequest.imr_multiaddr));
        multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

        if(setsockopt(sockfd, IPPROTO_IP , IP_ADD_MEMBERSHIP , (char*) &multicastRequest , sizeof(multicastRequest)) != 0) {
            perror("setsockopt failed");
            if(localAddr)
                freeaddrinfo(localAddr);
            if(mediaAddr)
                freeaddrinfo(mediaAddr);
            return -1;
        }
    }

    if(localAddr)
        freeaddrinfo(localAddr);
    if(mediaAddr)
        freeaddrinfo(mediaAddr);
    return sockfd;
}

int socket_utility_multicast_connect_socket(const char* multicastIP , const char* multicastPort) {
    int sockfd;
    struct addrinfo hints = { 0 };
    struct addrinfo* res = 0;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int addrInfoStatus;
    if((addrInfoStatus = getaddrinfo(multicastIP , multicastPort , &hints , &res)) != 0) {
        if(res)
            freeaddrinfo(res);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    if ((sockfd = socket(res->ai_family , res->ai_socktype , 0)) < 0) {
        perror("socket() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    // allow multiple sockets to use the same PORT number
    unsigned int reuse_port = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &reuse_port, sizeof(reuse_port)) < 0) {
        perror("setsockopt() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if(res)
        freeaddrinfo(res);
    return sockfd;
}

int socket_utility_ucast_connect_socket(const char* unicastIP , const char* unicastPort) {
    int sockfd;
    struct addrinfo hints = { 0 };
    struct addrinfo* res = 0;
    int yes = 1;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int addrInfoStatus;
    if((addrInfoStatus = getaddrinfo(unicastIP , unicastPort , &hints , &res)) != 0) {
        if(res)
            freeaddrinfo(res);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    if ((sockfd = socket(res->ai_family , res->ai_socktype , 0)) < 0) {
        perror("socket() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if(setsockopt(sockfd , SOL_SOCKET , SO_REUSEADDR , (char*)&yes , sizeof(int)) == -1) {
        perror("setsockopt() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if(res)
        freeaddrinfo(res);
    return sockfd;
}

int socket_utility_send_socket(const char* recvIP , const char* recvPort) {
    if(socket_utility_is_multicast_address(recvIP))
        return socket_utility_multicast_connect_socket(recvIP, recvPort);

    return socket_utility_ucast_connect_socket(recvIP, recvPort);
}

int socket_utility_maximum_of_three_num( int a , int b , int c ) {
   int max = ( a < b ) ? b : a;
   return ( ( max < c ) ? c : max );
}

socket_utility_t* socket_utility_create(const char* mediaIP , const char* mediaPort, const char* outputIP , const char* outputPort) {
    socket_utility_t *su = (socket_utility_t*)malloc(sizeof(socket_utility_t));
    if(!su) {
        perror("Failed to allocate socket_utility");
        exit(1);
    }

    char *fecRowPort = (char*)malloc(20);
    char *fecColPort = (char*)malloc(20);
    if (!fecRowPort || !fecColPort) exit(1);

    sprintf(fecRowPort , "%d" , atoi(mediaPort) + 4);
    sprintf(fecColPort , "%d" , atoi(mediaPort) + 2);

    su->media_Sockfd = socket_utility_listen_socket(mediaIP , mediaPort , SO_RCVBUF_SIZE);
    su->fecRow_Sockfd = socket_utility_listen_socket(mediaIP , fecRowPort , SO_RCVBUF_SIZE);
    su->fecCol_Sockfd = socket_utility_listen_socket(mediaIP , fecColPort , SO_RCVBUF_SIZE);
    su->output_Sockfd = socket_utility_send_socket(outputIP , outputPort);

    su->fdmax = socket_utility_maximum_of_three_num(su->media_Sockfd , su->fecRow_Sockfd , su->fecCol_Sockfd);

    su->sockRecvBuf = (unsigned char*)malloc(RECVBUFLEN * sizeof(unsigned char));

    free(fecRowPort);
    free(fecColPort);

    return su;
}

void socket_utility_destroy(socket_utility_t *su) {
    if(su) {
        if(su->media_Sockfd >= 0)
            close(su->media_Sockfd);
        if(su->fecRow_Sockfd >= 0)
            close(su->fecRow_Sockfd);
        if(su->fecCol_Sockfd >= 0)
            close(su->fecCol_Sockfd);
        if(su->output_Sockfd >= 0)
            close(su->output_Sockfd);
        if(su->sockRecvBuf)
            free(su->sockRecvBuf);
        free(su);
    }
}
