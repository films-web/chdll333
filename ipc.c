#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "protocol.h"
#include "queue.h"
#include "ipc.h"

HANDLE hPipe = INVALID_HANDLE_VALUE;

CH_Queue ipc_in_queue;
CH_Queue ipc_out_queue;

void IPC_QueueData(const CH_Packet* pkt) {
    Queue_Push(&ipc_out_queue, pkt);
}

int IPC_DequeueMessage(CH_Packet* outPkt) {
    return Queue_Pop(&ipc_in_queue, outPkt);
}

DWORD WINAPI IPC_Thread(LPVOID lpParam) {
    Queue_Init(&ipc_in_queue);
    Queue_Init(&ipc_out_queue);

    while (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) {
        hPipe = CreateFileA("\\\\.\\pipe\\CHPipe", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) {
            Sleep(1000);
        }
    }

    DWORD bytesRead = 0, bytesAvail = 0, bytesWritten = 0;

    // The compiler now knows hPipe is 100% valid for all functions inside this loop
    while (hPipe != INVALID_HANDLE_VALUE && hPipe != NULL) {
        CH_Packet outPkt;

        // 1. Sending Packets to Loader
        if (Queue_Pop(&ipc_out_queue, &outPkt)) {
            outPkt.magic = CH_MAGIC_WORD;

            DWORD toWrite = (sizeof(int) * 3) + outPkt.size;
            if (WriteFile(hPipe, &outPkt, toWrite, &bytesWritten, NULL) == FALSE) {
                break; // Pipe is dead, break loop to reconnect
            }
        }

        // 2. Receiving Packets from Loader
        if (PeekNamedPipe(hPipe, NULL, 0, NULL, &bytesAvail, NULL) != FALSE && bytesAvail > 0) {
            CH_Packet inPkt;

            // FIXED: Read the ENTIRE struct at once in a single ReadFile call
            if (ReadFile(hPipe, &inPkt, sizeof(CH_Packet), &bytesRead, NULL) != FALSE) {

                // Ensure we at least read the 12-byte header
                if (bytesRead >= (sizeof(int) * 3)) {

                    // Validate security signature
                    if (inPkt.magic == CH_MAGIC_WORD) {

                        // Validate payload size bounds
                        if (inPkt.size >= 0 && inPkt.size <= MAX_PAYLOAD_SIZE) {
                            Queue_Push(&ipc_in_queue, &inPkt);
                        }
                    }
                    else {
                        break; // Invalid magic word, break to drop connection
                    }
                }
            }
            else {
                break; // Read failed (buffer too small or pipe broke), break to reconnect
            }
        }

        Sleep(1);
    }

    // Safely clean up the handle outside the loop
    if (hPipe != INVALID_HANDLE_VALUE && hPipe != NULL) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    IPC_Init(); // Attempt to reconnect
    return 0;
}

void IPC_Init(void) {
    CreateThread(NULL, 0, IPC_Thread, NULL, 0, NULL);
}