#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "protocol.h"
#include "queue.h"

void Queue_Init(CH_Queue* q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    InitializeCriticalSection(&q->lock);
}

int Queue_Push(CH_Queue* q, const CH_Packet* pkt) {
    EnterCriticalSection(&q->lock);

    if (q->count >= QUEUE_MAX_MESSAGES) {
        LeaveCriticalSection(&q->lock);
        return 0;
    }

    q->messages[q->tail] = *pkt;
    q->tail = (q->tail + 1) % QUEUE_MAX_MESSAGES;
    q->count++;

    LeaveCriticalSection(&q->lock);
    return 1;
}

int Queue_Pop(CH_Queue* q, CH_Packet* outPkt) {
    EnterCriticalSection(&q->lock);

    if (q->count == 0) {
        LeaveCriticalSection(&q->lock);
        return 0;
    }

    *outPkt = q->messages[q->head];
    q->head = (q->head + 1) % QUEUE_MAX_MESSAGES;
    q->count--;

    LeaveCriticalSection(&q->lock);
    return 1;
}