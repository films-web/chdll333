#ifndef IPC_H
#define IPC_H

#include "protocol.h"

void IPC_Init(void);
void IPC_QueueData(const CH_Packet* pkt);
int IPC_DequeueMessage(CH_Packet* outPkt);

#endif