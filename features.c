#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "ipc.h"
#include "packet_builder.h"
#include "features.h"

extern void trap_RemoveCommand(const char* cmdName);

static CH_Context ctx = {
    0, 0, 0, -1
};

vmCvar_t ch_fullbright;
vmCvar_t ch_autocolor;

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

    CH_MonitorCvars();
}

void CH_MonitorCvars(void)
{
    char buffer[128];
    static char lastGlDriver[128] = { 0 };

    trap_Cvar_VariableStringBuffer("r_gldriver", buffer, sizeof(buffer));
    
    if (lastGlDriver[0] == '\0' && buffer[0] != '\0')
    {
        strncpy(lastGlDriver, buffer, sizeof(lastGlDriver) - 1);
        return;
    }

    if (lastGlDriver[0] != '\0' && strcmp(buffer, lastGlDriver) != 0)
    {
        char details[256];
        _snprintf(details, sizeof(details), "CVAR: r_gldriver changed from %s to %s (Blocked)", lastGlDriver, buffer);
        CH_ReportEvent("CVAR Attempted", "Blocked & Reset", details, "high");
        
        trap_Cvar_Set("r_gldriver", lastGlDriver);
    }
}


void CH_ReportEvent(const char* type, const char* action, const char* details, const char* severity)
{
    CH_EventPayload data;
    memset(&data, 0, sizeof(data));

    strncpy(data.eventType, type, sizeof(data.eventType) - 1);
    strncpy(data.action, action, sizeof(data.action) - 1);
    strncpy(data.details, details, sizeof(data.details) - 1);
    strncpy(data.severity, severity, sizeof(data.severity) - 1);

    if (cg)
    {
        trap_Cvar_VariableStringBuffer("name", data.playerName, sizeof(data.playerName) - 1);
    }
    else
    {
        strncpy(data.playerName, "Unknown", sizeof(data.playerName) - 1);
    }

    CH_SendPacket(CH_CMD_REPORT_EVENT, &data, sizeof(CH_EventPayload));
}


void CH_GameReady(void)
{
    CH_SendPacket(CH_CMD_GAME_READY, NULL, 0);
}

static void CH_Status_f(void)
{
    if (ctx.waitingForPlayerList) return;

    ctx.waitingForPlayerList = 1;

    CH_SendPacket(CH_CMD_REQUEST_PLAYER_LIST, NULL, 0);
    trap_Print("^3[CheatHaram] ^7Requesting active AC players from server...\n");
}

static void CH_Fairshot_f(void)
{
    if (ctx.waitingForFairshot) return;

    if (pCMD_Argc() < 2)
    {
        trap_Print("^3[CheatHaram] ^7Usage: ch_fs <#ID or Player GUID>\n");
        return;
    }

    char* target = pCMD_Argv(1);
    ctx.waitingForFairshot = 1;

    CH_SendPacket(CH_CMD_REQUEST_FAIRSHOT, target, (int)strlen(target));
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
            CH_ReportEvent("Command Used", "Blocked", "User tried to use 'hash' command", "high");
            return 1;
        }
    }

    return 0;
}


void CH_RequestInitData(void)
{
    CH_SendPacket(CH_CMD_REQUEST_SCAN, NULL, 0);
}

void CH_CheckIncomingChat(const char* text)
{
    if (!cg || ctx.autoReplyPending) return;

    if (IsTypedAlone(text, "@ch") || IsTypedAlone(text, "@fp") || IsTypedAlone(text, "@ac"))
    {
        ctx.autoReplyPending = 1;
        CH_SendPacket(CH_CMD_REQUEST_GUID, NULL, 0);
    }
}

void CH_UpdateState(void)
{
    CH_PlayerDataPayload data;
    memset(&data, 0, sizeof(data));

    if (cg)
    {
        data.inGame = 1;
        data.playerNum = cg->clientNum;
        trap_Cvar_VariableStringBuffer("name", data.name, sizeof(data.name));
        trap_Cvar_VariableStringBuffer("cl_currentServerAddress", data.server, sizeof(data.server));
    }
    else
    {
        data.inGame = 0;
        data.playerNum = -1;
        strncpy(data.name, "UnamedPlayer", sizeof(data.name) - 1);
        strncpy(data.server, "In Lobby", sizeof(data.server) - 1);
    }

    CH_SendPacket(CH_INFO_PLAYER_DATA, &data, sizeof(CH_PlayerDataPayload));
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
            pkt.payload[pkt.size] = '\0';
            trap_SendConsoleCommand((char*)pkt.payload);
            break;
        }
        case CH_CMD_SET_GUID:
        {
            pkt.payload[pkt.size] = '\0';
            trap_SendConsoleCommand((char*)pkt.payload);
            ctx.autoReplyPending = 0;
            break;
        }
        case CH_CMD_CONNECT_SERVER:
        {
            pkt.payload[pkt.size] = '\0';
            if (cg) {
                trap_SendConsoleCommand((char*)pkt.payload);
            }
            else {
                pCbuf_AddText((char*)pkt.payload);
            }
            break;
        }
        case CH_CMD_SET_PLAYER_LIST:
        {
            pkt.payload[pkt.size] = '\0';
            trap_Print("\n^3--- Active CheatHaram Players ---\n\n");
            trap_Print("^3#ID   GUID    NAME\n");
            trap_Print((char*)pkt.payload);
            trap_Print("\n^3----------------------------------\n");
            ctx.waitingForPlayerList = 0;
            break;
        }
        case CH_CMD_TOGGLECONSOLE:
        {
            pkt.payload[pkt.size] = '\0';
            if (trap_Key_GetCatcher() & 1)
            {
                pCbuf_AddText((char*)pkt.payload);
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
            pkt.payload[pkt.size] = '\0';
            trap_Print((char*)pkt.payload);
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