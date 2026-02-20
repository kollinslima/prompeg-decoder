#ifndef INCLUDED_PACKETBUFFER_H
#define INCLUDED_PACKETBUFFER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "packetQueue.h"

typedef struct packet_buffer_t {
    queue_t *emptyQueue;
    queue_t *mediaQueue;
    queue_t *fecQueue;
    uint16_t minSN;
    uint16_t lastSN;
    uint32_t currentTS;
    uint32_t mediaSSRC;
    int recovered;
} packet_buffer_t;

packet_buffer_t* packet_buffer_create(int quantity);
void packet_buffer_destroy(packet_buffer_t *pb);
void packet_buffer_free_node_to_empty_queue(packet_buffer_t *pb, node_t *target);
void packet_buffer_new_media_packet(packet_buffer_t *pb, const void *buffer, size_t length);
void packet_buffer_new_fec_packet(packet_buffer_t *pb, const void *buffer, size_t length);
void packet_buffer_update_fec_queue(packet_buffer_t *pb);
void packet_buffer_fec_delete_node(packet_buffer_t *pb, node_t *target, node_t *targetPrev, node_t *targetNext);
void packet_buffer_update_media_queue(packet_buffer_t *pb, int sockfd, int maxTimeRange);
void packet_buffer_update_min_sn(packet_buffer_t *pb);
void packet_buffer_buffer_monitor(packet_buffer_t *pb);
int packet_buffer_is_recoverable(packet_buffer_t *pb, uint16_t SNBase, uint8_t offset, uint8_t NA);
void packet_buffer_fec_recovery(packet_buffer_t *pb);
void packet_buffer_recover_packet(packet_buffer_t *pb, node_t *fecNode);
void packet_buffer_xor_slow(uint8_t *in1, uint8_t *in2, uint8_t *out, int size);

#endif
