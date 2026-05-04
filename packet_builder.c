#include <string.h>
#include "packet_builder.h"
#include "ipc.h"

void CH_SendPacket(unsigned int type, const void* data, unsigned int size)
{
    CH_Packet pkt = { 0 };

    if (size > MAX_PAYLOAD_SIZE)
        size = MAX_PAYLOAD_SIZE;

    pkt.magic = CH_MAGIC_WORD;
    pkt.type  = type;
    pkt.size  = size;

    if (data && size)
        memcpy(pkt.payload, data, size);

    IPC_QueueData(&pkt);
}