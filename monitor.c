#include "monitor.h"

stream_counter_t* stream_counter_create() {
    stream_counter_t *sc = (stream_counter_t*)malloc(sizeof(stream_counter_t));
    if (!sc) {
        perror("Failed to allocate stream_counter");
        exit(1);
    }
    sc->recvd = 0;
    sc->lost = 0;
    sc->latestSN = 0;
    return sc;
}

void stream_counter_destroy(stream_counter_t *sc) {
    if (sc) {
        free(sc);
    }
}

void stream_counter_print_status(stream_counter_t *sc) {
    double loss_rate = 0.0;
    if (sc->lost + sc->recvd > 0) {
        loss_rate = 100.0 * sc->lost / (double)(sc->lost + sc->recvd);
    }
    printf("    recvd: %d , lost: %d , loss rate: %lf\n", sc->recvd, sc->lost, loss_rate);
}

void stream_counter_update_status(stream_counter_t *sc, const void *buffer) {
    rtpHeader_ *rtpHeader = (rtpHeader_ *)buffer;
    uint16_t thisSN = ntohs(rtpHeader->sequenceNum);

    if (sc->latestSN > 0 && (thisSN - 1) > sc->latestSN) {
        int lostCounter = (thisSN - sc->latestSN) - 1;
        sc->lost += lostCounter;
    }

    sc->recvd++;
    sc->latestSN = thisSN;
    return;
}

monitor_t* monitor_create() {
    monitor_t *m = (monitor_t*)malloc(sizeof(monitor_t));
    if (!m) {
        perror("Failed to allocate monitor");
        exit(1);
    }
    m->media = stream_counter_create();
    m->fecRow = stream_counter_create();
    m->fecCol = stream_counter_create();
    m->recovered = 0;
    return m;
}

void monitor_destroy(monitor_t *m) {
    if (m) {
        stream_counter_destroy(m->media);
        stream_counter_destroy(m->fecRow);
        stream_counter_destroy(m->fecCol);
        free(m);
    }
}

void monitor_update_recovered(monitor_t *m, int newRecovered) {
    m->recovered = newRecovered;
}

void monitor_print_monitor(monitor_t *m) {
    printf("media stream:\n");
    stream_counter_print_status(m->media);

    double recovered_loss_ratio = 0.0;
    if (m->media->lost > 0) {
        recovered_loss_ratio = 100.0 * m->recovered / (double)m->media->lost;
    }
    printf("    recovered/lost: %d/%d = %lf\n", m->recovered, m->media->lost, recovered_loss_ratio);

    double loss_rate_after_recovery = 0.0;
    if (m->media->recvd + m->media->lost > 0) {
        loss_rate_after_recovery = 100.0 * (m->media->lost - m->recovered) / (double)(m->media->recvd + m->media->lost);
    }
    printf("    loss rate after recovery: %d/%d = %lf\n", (m->media->lost - m->recovered), (m->media->recvd + m->media->lost), loss_rate_after_recovery);

    printf("fecRow stream:\n");
    stream_counter_print_status(m->fecRow);
    printf("fecCol stream:\n");
    stream_counter_print_status(m->fecCol);
    printf("\n");
}
