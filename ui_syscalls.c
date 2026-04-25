#include "engine/ui_local.h"

extern void* args_list[11];
extern int (*ui_syscallptr)(int arg, ...);

static int ui_syscall(int cmd, ...)
{
	va_list args;
	int arg[12];
	int ret;
	int i, j;

	va_start(args, cmd);
	for (i = 10, j = 0; i > 0; i--) {
		args_list[j++] = va_arg(args, void*);
	}
	va_end(args);

	arg[0] = cmd;
	memcpy(arg + 1, args_list, sizeof(args_list));
	ret = ui_syscallptr((int)&arg);
	for (i = 0; i < 10; i++) {
		args_list[i] = 0;
	}
	return ret;
}

void trap_UI_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize) {
	ui_syscall(UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}