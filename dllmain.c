#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdint.h>
#include "ipc.h"
#include "features.h"


HWND(WINAPI* oCreateWindowExA)(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam) = NULL;

extern void init();

#define MakePtr(Type, Base, Offset) ((Type)(((DWORD)(Base)) + ((DWORD)(Offset))))

static BOOL Hook(HMODULE hLocalModule, const char* ModuleName, const char* FunctionName, PVOID xFunction, PVOID* oFunction)
{
    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hLocalModule;
    PIMAGE_NT_HEADERS pNTHeader;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;

    HMODULE hTargetModule = GetModuleHandleA(ModuleName);

    if (hTargetModule == NULL)
        return FALSE;

    DWORD dwAddressToIntercept = (DWORD)GetProcAddress(hTargetModule, FunctionName);

    if (!pDOSHeader || pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    pNTHeader = MakePtr(PIMAGE_NT_HEADERS, pDOSHeader, pDOSHeader->e_lfanew);
    if (!pNTHeader || pNTHeader->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    pImportDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, hLocalModule, pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    if (!pImportDesc)
        return FALSE;

    while (pImportDesc->Name)
    {
        PIMAGE_THUNK_DATA pThunk = MakePtr(PIMAGE_THUNK_DATA, hLocalModule, pImportDesc->FirstThunk);

        while (pThunk->u1.Function)
        {
            if ((DWORD)pThunk->u1.Function == dwAddressToIntercept)
            {
                DWORD oldProtect;
                if (VirtualProtect(&pThunk->u1.Function, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &oldProtect))
                {
                    if (oFunction)
                        *oFunction = (PVOID)pThunk->u1.Function;

                    pThunk->u1.Function = (DWORD)xFunction;

                    VirtualProtect(&pThunk->u1.Function, sizeof(DWORD), oldProtect, &oldProtect);
                    return TRUE;
                }
            }
            pThunk++;
        }
        pImportDesc++;
    }

    return FALSE;
}

static HWND WINAPI xCreateWindowExA(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam)
{
    if (lpWindowName && !strcmp(lpWindowName, "Soldier of Fortune 2 Console"))
    {
        init();
    }

    return oCreateWindowExA(
        dwExStyle,
        lpClassName,
        lpWindowName,
        dwStyle,
        X,
        Y,
        nWidth,
        nHeight,
        hWndParent,
        hMenu,
        hInstance,
        lpParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        IPC_Init();
        
        char processPath[MAX_PATH];
        GetModuleFileNameA(NULL, processPath, MAX_PATH);
        char* processName = strrchr(processPath, '\\');
        if (processName) processName++; else processName = processPath;

        if (_stricmp(processName, "SoF2.exe") != 0 && _stricmp(processName, "sof2mp.exe") != 0) {
            CH_ReportEvent("Security Violation", "Blocked", "Unauthorized DLL access (External Process)", "high");
        }

        Hook(

            GetModuleHandle(NULL),
            "user32.dll",
            "CreateWindowExA",
            (PVOID)&xCreateWindowExA,
            (PVOID*)&oCreateWindowExA);
    }
    return TRUE;
}