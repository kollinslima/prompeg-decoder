#include "packetQueue.h"

// Node functions
node_t* node_create() {
    node_t *n = (node_t*)malloc(sizeof(node_t));
    if (!n) {
        perror("Failed to allocate node");
        exit(1);
    }
    n->dataSize = 2048;
    n->dataUsed = 0;
    n->next = NULL;
    n->dataBuffer = (unsigned char*) malloc(2048 * sizeof(uint8_t));
    if (!n->dataBuffer) {
        perror("Failed to allocate node buffer");
        free(n);
        exit(1);
    }
    return n;
}

void node_free(node_t *n) {
    if (n) {
        if (n->dataBuffer) {
            free(n->dataBuffer);
        }
        free(n);
    }
}

uint32_t node_getTS(node_t *n) {
    if (n->dataUsed == 0) {
        return 0;
    } else {
        rtpPacket_ *rtpPacket = (rtpPacket_ *) n->dataBuffer;
        return ntohl(rtpPacket->rtpHeader.ts);
    }
}

uint16_t node_getSN(node_t *n) {
    if (n->dataUsed == 0) {
        return 0;
    } else {
        rtpPacket_ *rtpPacket = (rtpPacket_ *) n->dataBuffer;
        return ntohs(rtpPacket->rtpHeader.sequenceNum);
    }
}

uint16_t node_getSNBase(node_t *n) {
    if (n->dataUsed != 1344) {
        printf("getSNBase: %d\n", n->dataUsed);
        exit(1);
    } else {
        fecPacket_ *fecPacket = (fecPacket_ *) n->dataBuffer;
        return ntohs(fecPacket->fecHeader.SNBase);
    }
}

uint8_t node_getOffset(node_t *n) {
    if (n->dataUsed != 1344) {
        printf("getOffset\n");
        exit(1);
    } else {
        fecPacket_ *fecPacket = (fecPacket_ *) n->dataBuffer;
        return fecPacket->fecHeader.offset;
    }
}

uint8_t node_getNA(node_t *n) {
    if (n->dataUsed != 1344) {
        printf("getNA\n");
        exit(1);
    } else {
        fecPacket_ *fecPacket = (fecPacket_ *) n->dataBuffer;
        return fecPacket->fecHeader.NA;
    }
}

int node_isTsToolate(node_t *n, uint32_t currentTS, int maxTimeRange) {
    if (currentTS == 0) {
        return 0;
    } else {
        uint32_t thisTS = node_getTS(n);
        // Note: thisTS + maxTimeRange could overflow if types are not carefully handled, but original C++ did this.
        // Assuming wrapped arithmetic or large enough types.
        if (thisTS + maxTimeRange < currentTS) {
            return 1;
        } else {
            return 0;
        }
    }
}

int node_isPacketToolate(node_t *n, uint32_t currentTS, int maxTimeRange) {
    if (n->dataUsed > 0) {
        printf("gg: isPacketToolate()\n");
        exit(1);
    } else {
        int counter = 1;
        node_t *temp = n;
        while (temp->next) {
            temp = temp->next;
            if (temp->dataUsed > 0) {
                if (node_isTsToolate(temp, currentTS, maxTimeRange)) {
                    return counter;
                } else {
                    return 0;
                }
            } else {
                counter++;
            }
        }
        printf("fucked up\n");
        exit(1);
    }
}

// Queue functions
queue_t* queue_create() {
    queue_t *q = (queue_t*)malloc(sizeof(queue_t));
    if (!q) {
        perror("Failed to allocate queue");
        exit(1);
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

void queue_destroy(queue_t *q) {
    if (q) {
        free(q);
    }
}

int queue_isEmpty(queue_t *q) {
    if (q->size == 0) {
        return 1;
    } else {
        return 0;
    }
}

void queue_enqueue(queue_t *q, node_t *target) {
    if (target == NULL) {
        printf("target is NULL\n");
        exit(1);
    } else {
        target->next = NULL;
        if (queue_isEmpty(q)) {
            q->head = target;
        } else {
            q->tail->next = target;
        }
        q->tail = target;
        q->size++;
        return;
    }
}

node_t* queue_dequeue(queue_t *q) {
    if (queue_isEmpty(q)) {
        return NULL;
    } else {
        node_t* res = q->head;
        if (q->head->next) {
            q->head = q->head->next;
        } else {
            q->head = NULL;
        }
        q->size--;
        res->next = NULL;
        return res;
    }
}
