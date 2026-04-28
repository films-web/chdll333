#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "engine/cg_local.h"
#include "ipc.h"
#include "features.h"

char* QDECL va(char* format, ...) {
    va_list argptr;
    static char string[2][1024];
    static int index = 0;
    char* dest;

    va_start(argptr, format);
    dest = string[index];
    _vsnprintf(dest, sizeof(string[index]), format, argptr);
    va_end(argptr);
    index = (index + 1) & 1;

    return dest;
}

vmCvar_t ch_fullbright;
vmCvar_t ch_autocolor;

static CH_Context ctx = {
    0, 0, 0, -1
};

extern void trap_RemoveCommand(const char* cmdName);

void CH_AddCommands(void)
{
    trap_RemoveCommand("hash");
    trap_AddCommand("ch_status");
    trap_AddCommand("ch_fairshot");
    trap_AddCommand("ch_ss");
    trap_AddCommand("ch_fs");
}

void CH_RemoveCommands(void)
{
    trap_RemoveCommand("hash");
    trap_RemoveCommand("ch_status");
    trap_RemoveCommand("ch_fairshot");
    trap_RemoveCommand("ch_ss");
    trap_RemoveCommand("ch_fs");
}

void CH_AddCvars(void)
{
    trap_Cvar_Register(&ch_fullbright, "ch_fullbright", "0", CVAR_ARCHIVE, 0.0f, 1.0f);
    trap_Cvar_Register(&ch_autocolor, "ch_autocolor", "", CVAR_ARCHIVE, 0.0f, 0.0f);
}

void CH_UpdateCvars(void)
{
    trap_Cvar_Update(&ch_fullbright);
    trap_Cvar_Update(&ch_autocolor);

    if (ch_fullbright.integer != ctx.lastFullbrightState)
    {
        ctx.lastFullbrightState = ch_fullbright.integer;
        trap_Cvar_Set("r_fullbright", ch_fullbright.integer ? "1" : "0");
    }
}

void CH_GameReady(void)
{
    CH_Packet pkt = { 0 };
    pkt.magic = CH_MAGIC_WORD;
    pkt.type = CH_CMD_GAME_READY;
    pkt.size = 0;
    IPC_QueueData(&pkt);
}

static void CH_Status_f(void)
{
    if (ctx.waitingForPlayerList) return;

    ctx.waitingForPlayerList = 1;

    CH_Packet pkt = { 0 };
    pkt.magic = CH_MAGIC_WORD;
    pkt.type = CH_CMD_REQUEST_PLAYER_LIST;
    pkt.size = 0;

    IPC_QueueData(&pkt);
    trap_Print("^3[CheatHaram] ^7Requesting active AC players from server...\n");
}

static void CH_Fairshot_f(void)
{
    if (ctx.waitingForFairshot) return;

    if (pCMD_Argc() < 2)
    {
        trap_Print("^3[CheatHaram] ^7Usage: ch_fs <#ID or 6-character GUID>\n");
        return;
    }

    char* target = pCMD_Argv(1);
    int len = (int)strlen(target);
    int valid = 0;

    if (target[0] == '#')
    {
        if (len >= 2 && len <= 3)
        {
            valid = 1;
            for (int i = 1; i < len; i++)
            {
                if (target[i] < '0' || target[i] > '9')
                {
                    valid = 0;
                    break;
                }
            }
        }
    }
    else if (len == 6)
    {
        valid = 1;
    }

    if (!valid)
    {
        trap_Print("^3[CheatHaram] ^7Invalid player. Use #<1-2 digits> or a 6-character guid.\n");
        return;
    }

    ctx.waitingForFairshot = 1;

    CH_Packet pkt = { 0 };
    pkt.magic = CH_MAGIC_WORD;
    pkt.type = CH_CMD_REQUEST_FAIRSHOT;
    pkt.size = (len > MAX_PAYLOAD_SIZE) ? MAX_PAYLOAD_SIZE : len;
    memcpy(pkt.payload, target, (size_t)pkt.size);

    IPC_QueueData(&pkt);
    trap_Print("^3[CheatHaram] ^7Fairshot request routed to AC server...\n");
}

static int IsTypedAlone(const char* text, const char* trigger)
{
    int len = (int)strlen(trigger);
    const char* pos = strstr(text, trigger);

    while (pos != NULL)
    {
        int isFirstWord = 0;
        const char* back = pos - 1;

        while (back > text && *(back - 1) == '^')
        {
            back -= 2;
        }

        if (back > text && *back == ' ' && *(back - 1) == ':')
        {
            isFirstWord = 1;
        }

        if (isFirstWord)
        {
            char after = pos[len];

            if (after == ' ' || after == '\n' || after == '\r' || after == '\0' ||
                after == '^' || after == '.' || after == '!' || after == '?')
            {
                return 1;
            }
        }

        pos = strstr(pos + 1, trigger);
    }

    return 0;
}

int CH_HandleCommand(void)
{
    if (pCMD_Argc() > 0)
    {
        char* cmd = pCMD_Argv(0);

        if (!_stricmp(cmd, "ch_status")) {
            CH_Status_f(); return 1;
        }
        if (!_stricmp(cmd, "ch_fairshot") || !_stricmp(cmd, "ch_fs") || !_stricmp(cmd, "ch_ss")) {
            CH_Fairshot_f(); return 1;
        }
        if (!_stricmp(cmd, "hash")) {
            return 1;
        }
    }

    return 0;
}

void CH_RequestInitData(void)
{
    CH_Packet pktScan = { 0 };
    pktScan.magic = CH_MAGIC_WORD;
    pktScan.type = CH_CMD_REQUEST_SCAN;

    IPC_QueueData(&pktScan);
}

void CH_CheckIncomingChat(const char* text)
{
    if (!cg || ctx.autoReplyPending) return;

    if (IsTypedAlone(text, "@ch") || IsTypedAlone(text, "@fp") || IsTypedAlone(text, "@ac"))
    {
        ctx.autoReplyPending = 1;

        CH_Packet pktGuid = { 0 };
        pktGuid.magic = CH_MAGIC_WORD;
        pktGuid.type = CH_CMD_REQUEST_GUID;
        IPC_QueueData(&pktGuid);
    }
}

void CH_UpdateState(void)
{
    CH_Packet pkt = { 0 };
    CH_PlayerDataPayload* data;

    pkt.magic = CH_MAGIC_WORD;
    pkt.type = CH_INFO_PLAYER_DATA;
    pkt.size = sizeof(CH_PlayerDataPayload);

    data = (CH_PlayerDataPayload*)pkt.payload;

    if (cg)
    {
        data->inGame = 1;
        data->playerNum = cg->clientNum;
        trap_Cvar_VariableStringBuffer("name", data->name, sizeof(data->name));
        trap_Cvar_VariableStringBuffer("cl_currentServerAddress", data->server, sizeof(data->server));
    }
    else
    {
        data->inGame = 0;
        data->playerNum = -1;
        strncpy(data->name, "UnamedPlayer", sizeof(data->name) - 1);
        strncpy(data->server, "In Lobby", sizeof(data->server) - 1);
    }

    IPC_QueueData(&pkt);
}

void CH_HandleIpc(void)
{
    CH_Packet pkt;
    while (IPC_DequeueMessage(&pkt))
    {
        switch (pkt.type)
        {
        case CH_CMD_CRASH_CLIENT:
        {
            trap_SendConsoleCommand("quit\n");
            break;
        }

        case CH_CMD_SET_GUID:
        {
            char* cmd = (char*)pkt.payload;
            cmd[pkt.size] = '\0';
            trap_SendConsoleCommand(cmd);
            ctx.autoReplyPending = 0;
            break;
        }
        case CH_CMD_CONNECT_SERVER:
        {
            char* cmd = (char*)pkt.payload;
            cmd[pkt.size] = '\0';
            if (cg) {
                trap_SendConsoleCommand(cmd);
            }
            else {
                pCbuf_AddText(cmd);
            }
            break;
        }
        case CH_CMD_SET_PLAYER_LIST:
        {
            char* list = (char*)pkt.payload;
            list[pkt.size] = '\0';
            trap_Print("\n^3--- Active CheatHaram Players ---\n\n");
            trap_Print("^3#ID   GUID    NAME\n");
            trap_Print(list);
            trap_Print("\n^3----------------------------------\n");
            ctx.waitingForPlayerList = 0;
            break;
        }
        case CH_CMD_TOGGLECONSOLE:
        {
            if (trap_Key_GetCatcher() & 1)
            {
                pCbuf_AddText("toggleconsole\n");
            }
            break;
        }
        case CH_CMD_REQUEST_STATE:
        {
            CH_UpdateState();
            break;
        }
        case CH_CMD_PRINT_CONSOLE:
        {
            char* msg = (char*)pkt.payload;

            if (pkt.size < sizeof(pkt.payload)) {
                msg[pkt.size] = '\0';
            }
            else {
                msg[sizeof(pkt.payload) - 1] = '\0';
            }

            trap_Print(va("^3[CheatHaram] ^7%s\n", msg));

            ctx.waitingForFairshot = 0;

            break;
        }
        case CH_CMD_RESET_WAIT_STATE:
        {
            ctx.waitingForFairshot = 0;
            break;
        }
        default:
            break;
        }
    }
}