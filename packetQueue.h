#ifndef INCLUDED_PACKETQUEUE_H
#define INCLUDED_PACKETQUEUE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packetParser.h"

typedef struct node_t {
    int dataSize;
    int dataUsed;
    unsigned char *dataBuffer;
    struct node_t *next;
} node_t;

typedef struct queue_t {
    node_t *head;
    node_t *tail;
    int size;
} queue_t;

// Node functions
node_t* node_create();
void node_free(node_t *n);
uint32_t node_getTS(node_t *n);
uint16_t node_getSN(node_t *n);
uint16_t node_getSNBase(node_t *n);
uint8_t node_getOffset(node_t *n);
uint8_t node_getNA(node_t *n);
int node_isTsToolate(node_t *n, uint32_t currentTS, int maxTimeRange);
int node_isPacketToolate(node_t *n, uint32_t currentTS, int maxTimeRange);

// Queue functions
queue_t* queue_create();
void queue_destroy(queue_t *q);
int queue_isEmpty(queue_t *q);
void queue_enqueue(queue_t *q, node_t *target);
node_t* queue_dequeue(queue_t *q);

#endif
