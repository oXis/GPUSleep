#include "utils.h"
#include "cuda.h"

//         DST     ORI
std::map<LPVOID, LPVOID> heapLocationMap;

PIMAGE_NT_HEADERS GetNTHeaders(HMODULE imageBase)
{
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)imageBase;
	return (PIMAGE_NT_HEADERS)((SIZE_T)imageBase + dos_header->e_lfanew);
}

PIMAGE_OPTIONAL_HEADER GetOptionalHeader(HMODULE imageBase)
{
	return (PIMAGE_OPTIONAL_HEADER)((LPVOID)((SIZE_T)imageBase + ((PIMAGE_DOS_HEADER)(imageBase))->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)));
}

ULONG_PTR MoveDLLToGPUStrorage(HMODULE dll, PDWORD SizeOfHeaders, PNVIDIA_API_TABLE Api) {

	// Get headers
	DWORD oldProtect;
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)dll;
	PIMAGE_NT_HEADERS NTheader = GetNTHeaders((HMODULE)dll);

	// Allocate memory for the DLL
	ULONG_PTR storage = RtlAllocateGpuMemory(Api, NTheader->OptionalHeader.SizeOfImage + mapSize(heapMap));

	printf("RtlAllocateGpuMemory: %08x\n", storage);

	// copy headers to mem location
	*SizeOfHeaders = (DWORD)(dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders);
	Api->CudaMemoryCopyToDevice(storage, dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders);

	// Get first section
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(NTheader);

	// Copy all sections to memory
	for (int i = 0; i < NTheader->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD SectionSize = section->Misc.VirtualSize;
		printf("Section: %s - VirtualAddress %08x - VirtualSize %d - Moved to %08x\n", section->Name, (SIZE_T)dll + section->VirtualAddress, SectionSize, (ULONG_PTR)((SIZE_T)storage + section->VirtualAddress));

		ULONG_PTR dst = (ULONG_PTR)((SIZE_T)storage + section->VirtualAddress);
		Api->CudaMemoryCopyToDevice(dst, (byte*)dll + section->VirtualAddress, SectionSize);

		//zero out section
		VirtualProtect((LPVOID)((SIZE_T)dll + section->VirtualAddress), SectionSize, PAGE_READWRITE, &oldProtect);
		memset((LPVOID)((SIZE_T)dll + section->VirtualAddress), 0, SectionSize);
		VirtualProtect((LPVOID)((SIZE_T)dll + section->VirtualAddress), SectionSize, oldProtect, &oldProtect);
	}

	ULONG_PTR dst = (ULONG_PTR)((SIZE_T)storage + NTheader->OptionalHeader.SizeOfImage);
	for (auto it = heapMap.cbegin(); it != heapMap.cend(); ++it)
	{
		printf("Moved %08x to %08x\n", it->first, dst);

		heapLocationMap[(LPVOID)dst] = it->first;

		Api->CudaMemoryCopyToDevice((ULONG_PTR)dst, it->first, it->second);

		memset(it->first, 0, it->second); // zero out
		dst = (ULONG_PTR)((SIZE_T)dst + it->second);
	}

	//zero module headers
	VirtualProtect((LPVOID)dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &oldProtect);
	memset((LPVOID)dll, 0, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders);
	VirtualProtect((LPVOID)dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders, oldProtect, &oldProtect);

	return storage;
}

VOID MoveDLLFromGPUStrorage(HMODULE dll, ULONG_PTR storage, DWORD SizeOfHeaders, PNVIDIA_API_TABLE Api) {
	DWORD oldProtect;

	// Set mem to zero and copy headers to mem location
	VirtualProtect((LPVOID)dll, SizeOfHeaders, PAGE_READWRITE, &oldProtect);
	Api->CudaMemoryCopyToHost((PVOID)dll, storage, SizeOfHeaders);
	VirtualProtect((LPVOID)dll, SizeOfHeaders, oldProtect, &oldProtect);

	// Get headers
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)dll;
	PIMAGE_NT_HEADERS NTheader = GetNTHeaders((HMODULE)dll);

	// Get first section
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(NTheader);

	// Copy all sections to memory
	for (int i = 0; i < NTheader->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD SectionSize = section->Misc.VirtualSize;
		printf("Section: %s - VirtualAddress %08x - VirtualSize %d - Moved from %08x\n", section->Name, (SIZE_T)dll + section->VirtualAddress, SectionSize, (ULONG_PTR)((SIZE_T)storage + section->VirtualAddress));

		LPVOID dst = (void*)((SIZE_T)dll + section->VirtualAddress);
		VirtualProtect(dst, SectionSize, PAGE_READWRITE, &oldProtect);
		Api->CudaMemoryCopyToHost((PVOID)dst, storage + section->VirtualAddress, SectionSize);
		VirtualProtect(dst, SectionSize, oldProtect, &oldProtect);
	}

	for (auto it = heapLocationMap.cbegin(); it != heapLocationMap.cend(); ++it)
	{
		printf("Moved %08x to %08x\n", it->first, it->second);

		Api->CudaMemoryCopyToHost((PVOID)it->second, (ULONG_PTR)it->first, heapMap[it->second]);
	}

	heapLocationMap.clear();

	Api->CudaMemoryFree(storage);
}

// ########## TESTING FUNCTIONS #######
HMODULE MoveDLLToStrorage(HMODULE dll) {
	// Get headers
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)dll;
	PIMAGE_NT_HEADERS NTheader = GetNTHeaders((HMODULE)dll);

	// Allocate memory for the DLL
	HMODULE storage = (HMODULE)VirtualAlloc(0, NTheader->OptionalHeader.SizeOfImage + mapSize(heapMap), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	printf("VirtualAlloc: %08x\n", storage);

	// Set mem to zero and copy headers to mem location
	memset(storage, 0, NTheader->OptionalHeader.SizeOfImage);
	memcpy(storage, dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders);

	// Get headers from the new location
	dosHeader = (PIMAGE_DOS_HEADER)storage;
	NTheader = GetNTHeaders((HMODULE)storage);

	//zero module headers
	DWORD oldProtect;
	VirtualProtect((LPVOID)dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &oldProtect);
	memset((LPVOID)dll, 0, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders);
	VirtualProtect((LPVOID)dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders, oldProtect, &oldProtect);

	// Get first section
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(NTheader);

	// Copy all sections to memory
	for (int i = 0; i < NTheader->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD SectionSize = section->Misc.VirtualSize;
		printf("Section: %s - VirtualAddress %08x - VirtualSize %d\n", section->Name, (SIZE_T)dll + section->VirtualAddress, SectionSize);

		LPVOID dst = (void*)((SIZE_T)storage + section->VirtualAddress);
		memcpy(dst, (byte*)dll + section->VirtualAddress, SectionSize);

		//zero out section
		VirtualProtect((LPVOID)((SIZE_T)dll + section->VirtualAddress), SectionSize, PAGE_READWRITE, &oldProtect);
		memset((LPVOID)((SIZE_T)dll + section->VirtualAddress), 0, SectionSize);
		VirtualProtect((LPVOID)((SIZE_T)dll + section->VirtualAddress), SectionSize, oldProtect, &oldProtect);
	}

	LPVOID dst = (LPVOID)((SIZE_T)storage + NTheader->OptionalHeader.SizeOfImage);
	for (auto it = heapMap.cbegin(); it != heapMap.cend(); ++it)
	{
		printf("Moved %08x to %08x\n", it->first, dst);

		heapLocationMap[dst] = it->first;

		memcpy(dst, it->first, it->second);
		memset(it->first, 0, it->second); // zero out
		dst = (LPVOID)((SIZE_T)dst + it->second);
	}

	return storage;
}

VOID MoveDLLFromStrorage(HMODULE dll, HMODULE storage) {
	DWORD oldProtect;

	// Get headers
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)storage;
	PIMAGE_NT_HEADERS NTheader = GetNTHeaders((HMODULE)storage);

	// Set mem to zero and copy headers to mem location
	VirtualProtect((LPVOID)dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &oldProtect);
	memcpy(dll, storage, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders);
	VirtualProtect((LPVOID)dll, dosHeader->e_lfanew + NTheader->OptionalHeader.SizeOfHeaders, oldProtect, &oldProtect);

	// Get headers from the new location
	dosHeader = (PIMAGE_DOS_HEADER)dll;
	NTheader = GetNTHeaders((HMODULE)dll);

	// Get first section
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(NTheader);

	// Copy all sections to memory
	for (int i = 0; i < NTheader->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD SectionSize = section->Misc.VirtualSize;
		printf("Section: %s - VirtualAddress %08x - VirtualSize %d\n", section->Name, (SIZE_T)dll + section->VirtualAddress, SectionSize);

		LPVOID dst = (void*)((SIZE_T)dll + section->VirtualAddress);
		VirtualProtect(dst, SectionSize, PAGE_READWRITE, &oldProtect);
		memcpy(dst, (byte*)storage + section->VirtualAddress, SectionSize);
		VirtualProtect(dst, SectionSize, oldProtect, &oldProtect);
	}

	for (auto it = heapLocationMap.cbegin(); it != heapLocationMap.cend(); ++it)
	{
		printf("Moved %08x to %08x\n", it->first, it->second);

		memcpy(it->second, it->first, heapMap[it->second]);
	}

	heapLocationMap.clear();

	memset(storage, 0, NTheader->OptionalHeader.SizeOfImage + mapSize(heapMap));
	VirtualFree((LPVOID)storage, NTheader->OptionalHeader.SizeOfImage + mapSize(heapMap), MEM_FREE);
}
