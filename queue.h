#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_MAX_MESSAGES 32

typedef struct {
    CH_Packet messages[QUEUE_MAX_MESSAGES];
    int head;
    int tail;
    int count;
    CRITICAL_SECTION lock;
} CH_Queue;

void Queue_Init(CH_Queue* q);
int Queue_Push(CH_Queue* q, const CH_Packet* pkt);
int Queue_Pop(CH_Queue* q, CH_Packet* outPkt);

#endif