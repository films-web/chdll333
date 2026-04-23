#ifndef FEATURES_H
#define FEATURES_H

extern void (*pCom_Printf)(char* fmt, ...);
extern char* (*pCMD_Argv)(int n);
extern int (*pCMD_Argc)(void);

extern cg_t* cg;
extern cgs_t* cgs;
extern refdef_t* refdef;

extern vmCvar_t ch_fullbright;
extern vmCvar_t ch_showinfo;
extern vmCvar_t ch_autocolor;
extern vmCvar_t ch_autoreply;

typedef struct {
	char guid[64];
	char uiString[128];
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
void CH_HandleIcp(void);
void CH_DrawUI(void);

void CH_RequestInitData(void);
void CH_CheckIncomingChat(const char* text);

#endif