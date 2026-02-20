#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include "socketConnection.h"
#include "packetQueue.h"
#include "packetBuffer.h"
#include "monitor.h"
#define RECVBUFLEN 1500

void *threadproc(void *arg);

packet_buffer_t *myPacketBuffer;
monitor_t *myMonitor;

int main(int argc, char *argv[]) {
    myPacketBuffer = packet_buffer_create(2048);
    myMonitor = monitor_create();

    const char *mediaIP;
    const char *mediaPort;
    const char *maxDelay;
    const char *outputIP;
    const char *outputPort;
    unsigned char *sockRecvBuf = (unsigned char*)malloc(RECVBUFLEN * sizeof(unsigned char));

    mediaIP = "239.255.0.1";
    mediaPort = "20000";
    outputIP = "127.0.0.1";
    outputPort = "8000";
    maxDelay = "500";

    if(argc == 2) {
        maxDelay = argv[1];
    }
    else if(argc == 3) {
        mediaIP = argv[1];
        mediaPort = argv[2];
    }
    else if(argc == 4) {
        mediaIP = argv[1];
        mediaPort = argv[2];
        maxDelay = argv[3]; // Changed from argv[5] to argv[3] as it was likely a bug/typo in original C++ code (which had argv[5] for argc==4).
    }
    else if(argc == 5) {
        mediaIP = argv[1];
        mediaPort = argv[2];
        outputIP = argv[3];
        outputPort = argv[4];
    }
    else if(argc == 6) {
        mediaIP = argv[1];
        mediaPort = argv[2];
        outputIP = argv[3];
        outputPort = argv[4];
        maxDelay = argv[5];
    }

    socket_utility_t *mySocketUtility = socket_utility_create(mediaIP , mediaPort, outputIP , outputPort);

    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(mySocketUtility -> media_Sockfd , &master);
    FD_SET(mySocketUtility -> fecRow_Sockfd , &master);
    FD_SET(mySocketUtility -> fecCol_Sockfd , &master);

    read_fds = master;

    pthread_t tid;
    pthread_create(&tid , NULL , &threadproc , NULL);

    for(;;) {
        packet_buffer_update_fec_queue(myPacketBuffer);
        packet_buffer_fec_recovery(myPacketBuffer);
        packet_buffer_update_media_queue(myPacketBuffer, mySocketUtility -> output_Sockfd , atoi(maxDelay) * 100 );
        packet_buffer_update_min_sn(myPacketBuffer);

        monitor_update_recovered(myMonitor, myPacketBuffer -> recovered);

        read_fds = master;
        struct timeval tv = {0 , 50};
        select( (mySocketUtility -> fdmax)+1 , &read_fds , NULL , NULL , &tv);

        if(FD_ISSET(mySocketUtility -> media_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recv(mySocketUtility -> media_Sockfd , sockRecvBuf , RECVBUFLEN , 0)) < 0) {
                socket_utility_destroy(mySocketUtility);
                exit(1);
            }

            stream_counter_update_status(myMonitor -> media, sockRecvBuf);
            packet_buffer_new_media_packet(myPacketBuffer, sockRecvBuf , bytes);
        }

        if(FD_ISSET(mySocketUtility -> fecRow_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recv(mySocketUtility -> fecRow_Sockfd , sockRecvBuf , RECVBUFLEN , 0)) < 0) {
                socket_utility_destroy(mySocketUtility);
                exit(1);
            }

            stream_counter_update_status(myMonitor -> fecRow, sockRecvBuf);
            packet_buffer_new_fec_packet(myPacketBuffer, sockRecvBuf , bytes);
        }

        if(FD_ISSET(mySocketUtility -> fecCol_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recv(mySocketUtility -> fecCol_Sockfd , sockRecvBuf , RECVBUFLEN , 0)) < 0) {
                socket_utility_destroy(mySocketUtility);
                exit(1);
            }

            stream_counter_update_status(myMonitor -> fecCol, sockRecvBuf);
            packet_buffer_new_fec_packet(myPacketBuffer, sockRecvBuf , bytes);
        }
    }

    return 0; //never reached
}

void *threadproc(void *arg) {
    while(1) {
        sleep(5);
        packet_buffer_buffer_monitor(myPacketBuffer);
        monitor_print_monitor(myMonitor);
    }
    return 0;
}
