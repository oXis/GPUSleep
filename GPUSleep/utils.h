#pragma once
#include <map>
#include <windows.h>
#include <iostream>
#include <intrin.h>
#include <psapi.h>

#include "minihook.h"
#include "cuda.h"

//extern PROCESS_HEAP_ENTRY entry;
extern DWORD GlobalThreadId;
extern LPVOID(WINAPI* OldHeapAlloc)(HANDLE, DWORD, SIZE_T);
extern void (WINAPI* OldSleep)(DWORD);
extern std::map<LPVOID, DWORD> heapMap;

extern NVIDIA_API_TABLE Api;
extern CUDA_CONTEXT Context;
extern HMODULE dll;

void run1(std::map<LPVOID, DWORD> heapMap);
typedef VOID(*typeFunc)();

void xor_bidirectional_encode(const char* key, const size_t keyLength, char* buffer, const size_t length);
void print_map(const std::map<LPVOID, DWORD>& m);
DWORD mapSize(const std::map<LPVOID, DWORD>& m);
void HeapEncryptDecryptSection(char* buffer, const size_t length);
void HeapEncryptDecrypt();
void HeapEncryptMap(const std::map<LPVOID, DWORD>& m);

LPVOID HookedHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
void HookedSleep(DWORD dwMilliseconds);

template <typename T>
inline MH_STATUS MH_CreateHookApiEx(
	LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHookApi(
		pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}