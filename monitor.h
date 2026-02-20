#ifndef INCLUDED_MONITOR_H
#define INCLUDED_MONITOR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "packetParser.h"

typedef struct stream_counter_t {
    int recvd;
    int lost;
    uint16_t latestSN;
} stream_counter_t;

typedef struct monitor_t {
    stream_counter_t *media;
    stream_counter_t *fecRow;
    stream_counter_t *fecCol;
    int recovered;
} monitor_t;

// StreamCounter functions
stream_counter_t* stream_counter_create();
void stream_counter_destroy(stream_counter_t *sc);
void stream_counter_print_status(stream_counter_t *sc);
void stream_counter_update_status(stream_counter_t *sc, const void *buffer);

// Monitor functions
monitor_t* monitor_create();
void monitor_destroy(monitor_t *m);
void monitor_update_recovered(monitor_t *m, int newRecovered);
void monitor_print_monitor(monitor_t *m);

#endif
