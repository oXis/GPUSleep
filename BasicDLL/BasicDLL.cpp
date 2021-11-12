#include <Windows.h>
#include <iostream>
#include <string>

unsigned char encdata[64] = "THIS IS AN ENCRYPTED STRING";
unsigned char* data;

void SetupConsole() {
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

extern "C" VOID __declspec(dllexport) Init()
{
	data = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 64);
	memcpy(data, encdata, 64);

	SetupConsole();

	printf("%s\n", encdata);

	memset(data, 0, 64);
	std::string str = "THIS IS AN DECRYPTED STRING";

	memcpy(data, str.c_str(), 64);

	//MessageBoxA(NULL, "Init Done", "Init", MB_OK);
}

extern "C" VOID __declspec(dllexport) Run()
{
	for (int i = 0; i < 2; i++) {
		printf("%s\n", data);
		Sleep(1000);
	}
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID)
{

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		MessageBoxA(NULL, "DLL_PROCESS_ATTACH", "Hello", MB_OK);
		Init();
	}
	break;
	case DLL_PROCESS_DETACH:
	{
		MessageBoxA(NULL, "DLL_PROCESS_DETACH", "Hello", MB_OK);
	}
	break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}