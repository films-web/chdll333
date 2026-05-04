#ifndef FEATURES_H
#define FEATURES_H

#include "engine/cg_local.h"

extern void (*pCbuf_AddText)(const char* text);
extern void (*pCom_Printf)(char* fmt, ...);
extern char* (*pCMD_Argv)(int n);
extern int (*pCMD_Argc)(void);

extern cg_t* cg;
extern cgs_t* cgs;
extern refdef_t* refdef;

extern vmCvar_t ch_fullbright;
extern vmCvar_t ch_autocolor;

typedef struct {
	int waitingForPlayerList;
	int waitingForFairshot;
	int autoReplyPending;
	int lastFullbrightState;
} CH_Context;

void CH_AddCommands(void);
void CH_RemoveCommands(void);
void CH_AddCvars(void);
void CH_UpdateCvars(void);
int  CH_HandleCommand(void);
void CH_HandleIpc(void);
void CH_GameReady(void);
void CH_RequestInitData(void);
void CH_CheckIncomingChat(const char* text);
void CH_UpdateState(void);

#endif