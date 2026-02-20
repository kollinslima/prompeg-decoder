#include "packetBuffer.h"

packet_buffer_t* packet_buffer_create(int quantity) {
    packet_buffer_t *pb = (packet_buffer_t*)malloc(sizeof(packet_buffer_t));
    if (!pb) exit(1);

    pb->emptyQueue = queue_create();
    pb->mediaQueue = queue_create();
    pb->fecQueue = queue_create();

    for(int i = 0 ; i < quantity ; i++) {
        node_t* temp = node_create();
        queue_enqueue(pb->emptyQueue, temp);
    }

    pb->minSN = 0;
    pb->lastSN = 0;
    pb->currentTS = 0;
    pb->mediaSSRC = 0;
    pb->recovered = 0;

    return pb;
}

void packet_buffer_destroy(packet_buffer_t *pb) {
    if (pb) {
        queue_destroy(pb->emptyQueue);
        queue_destroy(pb->mediaQueue);
        queue_destroy(pb->fecQueue);
        free(pb);
    }
}

void packet_buffer_free_node_to_empty_queue(packet_buffer_t *pb, node_t *target) {
    if(target == NULL) {
        printf("target is NULL\n");
        exit(1);
    }
    else {
        target -> next = NULL;
        target -> dataUsed = 0;
        queue_enqueue(pb->emptyQueue, target);
    }
}

void packet_buffer_new_media_packet(packet_buffer_t *pb, const void *buffer, size_t length) {
    rtpPacket_ *rtpPacket = (rtpPacket_*) buffer;
    uint16_t currentSN = ntohs(rtpPacket -> rtpHeader.sequenceNum);

    if( queue_isEmpty(pb->mediaQueue) || pb->lastSN == 0 || pb->minSN == 0 || (pb->minSN > currentSN) ) {
        pb->minSN = currentSN;
    }
    if( (currentSN - 1) > pb->lastSN && pb->lastSN != 0 ) {
        int emptyCounter = (currentSN - pb->lastSN) - 1;
        for(int i = 0 ; i < emptyCounter ; i++) {
            node_t *temp = queue_dequeue(pb->emptyQueue);
            temp -> dataUsed = 0;
            temp -> next = NULL;
            queue_enqueue(pb->mediaQueue, temp);
        }
    }

    pb->lastSN = currentSN;
    pb->currentTS = ntohl(rtpPacket -> rtpHeader.ts);
    if(pb->mediaSSRC == 0) {
        pb->mediaSSRC = ntohl(rtpPacket -> rtpHeader.ssrc);
    }

    node_t *temp = queue_dequeue(pb->emptyQueue);
    temp -> dataUsed = length;
    temp -> next = NULL;
    memcpy(temp -> dataBuffer , buffer , length);
    queue_enqueue(pb->mediaQueue, temp);
}

void packet_buffer_new_fec_packet(packet_buffer_t *pb, const void *buffer, size_t length) {
    node_t *temp = queue_dequeue(pb->emptyQueue);
    temp -> dataUsed = length;
    temp -> next = NULL;
    memcpy(temp -> dataBuffer , buffer , length);
    queue_enqueue(pb->fecQueue, temp);
}

void packet_buffer_fec_delete_node(packet_buffer_t *pb, node_t *target, node_t *targetPrev, node_t *targetNext) {
    if(!targetPrev && targetNext) {
        pb->fecQueue -> head = targetNext;
    }
    else if(!targetPrev && !targetNext) {
        pb->fecQueue -> head = NULL;
        pb->fecQueue -> tail = NULL;
    }
    else if(targetPrev && targetNext) {
        targetPrev -> next = targetNext;
    }
    else if(targetPrev && !targetNext) {
        targetPrev -> next = NULL;
        pb->fecQueue -> tail = targetPrev;
    }
    pb->fecQueue -> size--;
    packet_buffer_free_node_to_empty_queue(pb, target);
}

void packet_buffer_update_fec_queue(packet_buffer_t *pb) {
    if( pb->minSN == 0 || queue_isEmpty(pb->fecQueue) ) {
        return;
    }
    else {
        node_t *temp = pb->fecQueue -> head;
        node_t *tempPrev = NULL;
        node_t *tempNext = NULL;
        temp = pb->fecQueue -> head;

        while(temp) {
            if( node_getSNBase(temp) < pb->minSN ) {
                if(!temp -> next) {
                    tempPrev = tempPrev;
                    tempNext = NULL;
                    packet_buffer_fec_delete_node(pb, temp , tempPrev , NULL);
                    break;
                }
                else {
                    tempPrev = tempPrev;
                    tempNext = temp -> next;
                    packet_buffer_fec_delete_node(pb, temp , tempPrev , tempNext);
                    temp = tempNext;
                    continue;
                }
            }
            else {
                if(!temp -> next) {
                    break;
                }
                else {
                    tempPrev = temp;
                    tempNext = temp -> next;
                    temp = tempNext;
                    continue;
                }
            }
        }

        return;
    }
}

void packet_buffer_update_media_queue(packet_buffer_t *pb, int sockfd, int maxTimeRange) {
    if( pb->currentTS == 0 || queue_isEmpty(pb->mediaQueue) ) {
        return;
    }
    else {
        node_t *temp = pb->mediaQueue -> head;
        while(temp) {
            if(temp -> dataUsed == 0) {
                int emptyCounter = node_isPacketToolate(temp, pb->currentTS , maxTimeRange);
                if(emptyCounter == 0) {
                    return;
                }
                else {
                    for (int i = 0 ; i < emptyCounter ; i++) {
                        temp = queue_dequeue(pb->mediaQueue);
                        packet_buffer_free_node_to_empty_queue(pb, temp);
                    }
                    temp = pb->mediaQueue -> head;
                }
            }
            else {
                int lateFlag = node_isTsToolate(temp, pb->currentTS , maxTimeRange);
                if(lateFlag == 0) {
                    return;
                }
                else {
                    temp = queue_dequeue(pb->mediaQueue);
                    if (send(sockfd , temp -> dataBuffer , temp -> dataUsed , 0) == -1);
                        //perror("send");
                    packet_buffer_free_node_to_empty_queue(pb, temp);
                    temp = pb->mediaQueue -> head;
                }
            }
        }

        return;
    }
}

void packet_buffer_update_min_sn(packet_buffer_t *pb) {
    node_t *temp = pb->mediaQueue -> head;
    while(temp) {
        if(temp -> dataUsed == 0 && temp -> next) {
            temp = temp -> next;
        }
        else if( temp -> dataUsed == 0 && !(temp -> next) ) {
            pb->minSN = 0;
            return;
        }
        else if(temp -> dataUsed > 0) {
            pb->minSN = node_getSN(temp);
            return;
        }
    }
}

void packet_buffer_buffer_monitor(packet_buffer_t *pb) {
    printf("emptyQueue size: %d\n", pb->emptyQueue -> size);
    printf("mediaQueue size: %d\n", pb->mediaQueue -> size);
    printf("fecQueue size: %d\n\n", pb->fecQueue -> size);
}

int packet_buffer_is_recoverable(packet_buffer_t *pb, uint16_t SNBase, uint8_t offset, uint8_t NA) {
    node_t *temp = pb->mediaQueue -> head;
    int emptyCounter = 0;
    if(temp) {
        int differ = SNBase - node_getSN(temp);
        if(differ < 0) {
            //printf("gg , differ < 0: %d\n" , differ);
            printf("SNBase:%d , minSN:%d , headSN:%d\n", SNBase , pb->minSN , node_getSN(temp));
            exit(1);
        }
        else {
            for(int i = 0 ; i < differ ; i++) {
                if(temp -> next) {
                    temp = temp -> next;
                }
                else {
                    return -1;
                }
            }

            if(temp -> dataUsed == 0) {
                emptyCounter++;
            }

            for(int i = 0 ; i < NA - 1 ; i++) {
                for(int j = 0 ; j < offset ; j++) {
                    if(temp -> next) {
                        temp = temp -> next;
                    }
                    else {
                        return -1;
                    }
                }
                if(temp -> dataUsed == 0) {
                    emptyCounter++;
                }
            }
            return emptyCounter;
        }
    }
    return -1;
}

void packet_buffer_xor_slow(uint8_t *in1, uint8_t *in2, uint8_t *out, int size) {
    for (int i = 0 ; i < size ; i++) {
        out[i] = in1[i] ^ in2[i];
    }
}

void packet_buffer_recover_packet(packet_buffer_t *pb, node_t *fecNode) {
    fecPacket_ *fecPacket = (fecPacket_ *) fecNode -> dataBuffer;

    node_t *target = NULL;
    uint16_t targetSN = 0;
    //fecPacket -> fecHeader.tsRecovery
    //fecPacket -> payload

    node_t *temp = pb->mediaQueue -> head;
    int differ = node_getSNBase(fecNode) - node_getSN(temp);
    for(int i = 0 ; i < differ ; i++) {
        temp = temp -> next;
    }

    if(temp -> dataUsed == 0) {
        target = temp;
        targetSN = node_getSNBase(fecNode);
    }
    else {
        //xor
        rtpPacket_ *tempRTP = (rtpPacket_ *) temp -> dataBuffer;
        fecPacket -> fecHeader.tsRecovery ^= tempRTP -> rtpHeader.ts;
        packet_buffer_xor_slow(fecPacket -> payload , tempRTP -> payload , fecPacket -> payload , 1316);
    }

    for(int i = 0 ; i < node_getNA(fecNode) - 1 ; i++) {
        for(int j = 0 ; j < node_getOffset(fecNode) ; j++) {
            temp = temp -> next;
        }
        if(temp -> dataUsed == 0) {
            target = temp;
            targetSN = node_getSNBase(fecNode) + (i+1)*(node_getOffset(fecNode));
        }
        else {
            //xor
            rtpPacket_ *tempRTP = (rtpPacket_ *) temp -> dataBuffer;
            fecPacket -> fecHeader.tsRecovery ^= tempRTP -> rtpHeader.ts;
            packet_buffer_xor_slow(fecPacket -> payload , tempRTP -> payload , fecPacket -> payload , 1316);
        }
    }

    if(target != NULL) {
        target -> dataUsed = 1328;
        rtpPacket_ *rtpPacket = (rtpPacket_ *) target -> dataBuffer;
        rtpPacket -> rtpHeader.cc = 0;
        rtpPacket -> rtpHeader.extension = 0;
        rtpPacket -> rtpHeader.padding = 0;
        rtpPacket -> rtpHeader.version = 2;
        rtpPacket -> rtpHeader.pt = 33;
        rtpPacket -> rtpHeader.marker = 0;
        rtpPacket -> rtpHeader.sequenceNum = htons(targetSN);
        rtpPacket -> rtpHeader.ts = fecPacket -> fecHeader.tsRecovery;
        rtpPacket -> rtpHeader.ssrc = htonl(pb->mediaSSRC);
        memcpy ( rtpPacket -> payload , fecPacket -> payload , 1316 );

        pb->recovered++;
    }
    else {
        printf("gg\n");
    }
}

void packet_buffer_fec_recovery(packet_buffer_t *pb) {
    for(;;) {
        if(queue_isEmpty(pb->fecQueue)) {
            return;
        }
        else {
            int FINflag = 0;
            node_t *temp = pb->fecQueue -> head;
            node_t *tempPrev = NULL;
            node_t *tempNext = NULL;
            while(temp && temp -> dataUsed > 0) {
                int recoverFlag = packet_buffer_is_recoverable(pb, node_getSNBase(temp) , node_getOffset(temp) , node_getNA(temp));
                if(recoverFlag == 0) {
                    //delete
                    if(!temp -> next) {
                        tempPrev = tempPrev;
                        tempNext = NULL;
                        packet_buffer_fec_delete_node(pb, temp , tempPrev , NULL);
                        break;
                    }
                    else {
                        tempPrev = tempPrev;
                        tempNext = temp -> next;
                        packet_buffer_fec_delete_node(pb, temp , tempPrev , tempNext);
                        temp = tempNext;
                        continue;
                    }
                }
                else if(recoverFlag == 1) {
                    //recover and delete
                    packet_buffer_recover_packet(pb, temp);
                    FINflag++;

                    if(!temp -> next) {
                        tempPrev = tempPrev;
                        tempNext = NULL;
                        packet_buffer_fec_delete_node(pb, temp , tempPrev , NULL);
                        break;
                    }
                    else {
                        tempPrev = tempPrev;
                        tempNext = temp -> next;
                        packet_buffer_fec_delete_node(pb, temp , tempPrev , tempNext);
                        temp = tempNext;
                        continue;
                    }
                }
                else if(recoverFlag > 1 || recoverFlag == -1) {
                    tempPrev = temp;
                    if(!temp -> next) {
                        tempNext = NULL;
                        break;
                    }
                    else {
                        tempNext = temp -> next;
                        temp = tempNext;
                        continue;
                    }
                }
            }

            if(FINflag == 0) {
                return;
            }
        }
    }
}
