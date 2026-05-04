#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "engine/ui_public.h"

int cgvm;
int uivm;
int (*syscallptr)(int arg, ...) = (int (*)(int, ...)) - 1;
int (*pVM_Create)(char*, int (*)(int*), int, int);
int (*pVM_Call)(int vm, int command, ...);
void* (*pVM_GetArg)(void* arg);
void* args_list[11];
void (*pCom_Printf)(char* fmt, ...);
char* (*pCMD_Argv)(int n);
int (*pCMD_Argc)(void);
void (*o_CL_AddReliableCommand)(const char* cmd);
void (*pCbuf_AddText)(const char* text);

cg_t* cg = 0;
cgs_t* cgs = 0;
refdef_t* refdef = 0;

static void x_CL_AddReliableCommand(const char* cmd) {

	if (cmd[0] != 's' || !ch_autocolor.string || !ch_autocolor.string[0]) {
		o_CL_AddReliableCommand(cmd);
		return;
	}

	int offset =
		!strncmp(cmd, "say ", 4) ? 4 :
		!strncmp(cmd, "say_team ", 9) ? 9 : 0;

	if (!offset) {
		o_CL_AddReliableCommand(cmd);
		return;
	}

	if (cmd[offset] == '"')
		offset++;

	char newCmd[1024];
	const char* src = cmd, * end = cmd + offset;
	char* dst = newCmd;

	while (src < end)
		*dst++ = *src++;

	if (*src != '^') {
		*dst++ = '^';
		*dst++ = ch_autocolor.string[0];
	}

	while ((*dst++ = *src++) != '\0');

	o_CL_AddReliableCommand(newCmd);
}

static int CG_syscalls(int* args)
{
	switch (args[0])
	{
	case CG_GETGAMESTATE:
	{
		gameState_t* gstate = (gameState_t*)pVM_GetArg((void*)args[1]);
		if (!cgs)
		{
			cgs = (cgs_t*)((char*)gstate - offsetof(cgs_t, gameState));
		}
	}
	break;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		if (!cg)
		{
			cg = (cg_t*)((char*)pVM_GetArg((void*)args[2]) - offsetof(cg_t, latestSnapshotTime));
		}
		break;
	case CG_GETGLCONFIG:
		if (!cgs)
		{
			cgs = (cgs_t*)((char*)pVM_GetArg((void*)args[1]) - offsetof(cgs_t, glconfig));
		}
		break;
	case CG_R_RENDERSCENE:
		if (!refdef)
		{
			refdef = (refdef_t*)pVM_GetArg((void*)args[1]);
		}
		break;

	case CG_PRINT:
	{
		const char* text = (const char*)pVM_GetArg((void*)args[1]);
		if (text) {
			CH_CheckIncomingChat(text);
		}
	}
	break;

	default:
		break;
	}

	return syscallptr((int)args);
}

static int VM_Create(char* module, int (*psyscallptr)(int*), int interpret, int bruh)
{
	if (!strcmp(module, "sof2mp_ui"))
	{
		uivm = (*pVM_Create)(module, psyscallptr, interpret, bruh);
		return uivm;
	}

	else if (!strcmp(module, "sof2mp_cgame"))
	{
		syscallptr = (int (*)(int, ...))psyscallptr;
		cgvm = (*pVM_Create)(module, &CG_syscalls, interpret, bruh);
		return cgvm;
	}

	return (*pVM_Create)(module, psyscallptr, interpret, bruh);
}

static int VM_Call(int vm, int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9)
{
	if (vm == cgvm)
	{
		switch (command)
		{
		case CG_DRAW_ACTIVE_FRAME:
			(*pVM_Call)(vm, CG_DRAW_ACTIVE_FRAME, arg0, arg1, arg2);
			CH_UpdateCvars();
			CH_HandleIpc();
			return 0;

		case CG_INIT:
			(*pVM_Call)(vm, CG_INIT, arg0, arg1, arg2);

			CH_AddCommands();
			CH_AddCvars();
			CH_RequestInitData();
			CH_UpdateState();


			return 0;

		case CG_CONSOLE_COMMAND:
			if (CH_HandleCommand()) {
				return 1;
			}

			return (*pVM_Call)(vm, CG_CONSOLE_COMMAND);

		case CG_SHUTDOWN:
			cg = 0;
			cgs = 0;
			refdef = 0;
			CH_RemoveCommands();
			CH_UpdateState();
			return (*pVM_Call)(vm, CG_SHUTDOWN);

		default:
			return (*pVM_Call)(vm, command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
		}
	}

	else if (vm == uivm)
	{
		switch (command)
		{

		case UI_REFRESH:
			(*pVM_Call)(vm, UI_REFRESH, arg0);
			CH_HandleIpc();
			return 0;

		case UI_INIT:
			(*pVM_Call)(vm, UI_INIT, arg0);
			CH_GameReady();
			return 0;

		case UI_SHUTDOWN:
			return (*pVM_Call)(vm, UI_SHUTDOWN);

		default:
			return (*pVM_Call)(vm, command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
		}
	}

	return (*pVM_Call)(vm, command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

static void* VM_GetArg(void* arg)
{
	int i;
	if (arg)
	{
		for (i = 0; i < 10; i++)
		{
			if (args_list[i] == arg)
			{
				return arg;
			}
		}
	}
	return pVM_GetArg(arg);
}

static void* detour(void* src, void* dest, int len)
{
	unsigned char* trampo;
	unsigned char* tmp;
	DWORD oldprotect;

	if (len < 5)
	{
		return 0;
	}

	trampo = (unsigned char*)malloc(len + 5);

	if (trampo == NULL) {
		return 0;
	}

	memcpy(trampo, src, len);
	trampo[len] = 0xE9;
	*((void**)(trampo + len + 1)) = (void*)((((unsigned int)src) + len) - (unsigned int)(trampo + len + 5));

	VirtualProtect((void*)(((unsigned long)src) & 0xfffff000), 4096, PAGE_EXECUTE_READWRITE, &oldprotect);
	if ((((unsigned int)src) & 0xfff) > 0xff9)
	{
		VirtualProtect((void*)(((unsigned long)src + 4096) & 0xfffff000), 4096, PAGE_EXECUTE_READWRITE, &oldprotect);
	}

	*((unsigned char*)src) = 0xE9;
	*((void**)((unsigned int)src + 1)) = (void*)(((unsigned int)dest) - (((unsigned int)src) + 5));

	tmp = (unsigned char*)(((unsigned int)src) + 5);
	for (; len > 5; len--, tmp++)
	{
		*tmp = 0xCC;
	}

	return trampo;
}

void init()
{
	DWORD baseAddr = (DWORD)GetModuleHandle(NULL);

	pVM_Create = (int (*)(char*, int (*)(int*), int, int))detour((void*)(baseAddr + 0x5E610), &VM_Create, 8);
	pVM_Call = (int (*)(int, int, ...))detour((void*)(baseAddr + 0x5EB10), &VM_Call, 8);
	pVM_GetArg = (void* (*)(void*))detour((void*)(baseAddr + 0x5EAB0), &VM_GetArg, 6);
	pCbuf_AddText = (void (*)(const char*))(baseAddr + 0x44380);
	pCMD_Argv = (char* (*)(int))(baseAddr + 0x448C0);
	pCMD_Argc = (int (*)(void))(baseAddr + 0x448B0);
	pCom_Printf = (void (*)(char*, ...))(baseAddr + 0x45320);
	o_CL_AddReliableCommand = (void (*)(const char*))detour((void*)(baseAddr + 0xC3F0), &x_CL_AddReliableCommand, 5);
}