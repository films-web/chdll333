#include "engine/ui_local.h"

extern void* args_list[11];
extern int (*syscallptr) (int arg, ...);

extern int syscall(int cmd, ...);

void trap_UI_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize) {
	syscall(UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}