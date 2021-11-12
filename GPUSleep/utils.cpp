#include <map>
#include <windows.h>
#include <iostream>
#include <intrin.h>
#include <psapi.h>

#include "minihook.h"

PROCESS_HEAP_ENTRY entry;

const char key[9] = "FROMAGE";
const size_t keySize = sizeof(key);

#pragma intrinsic(_ReturnAddress)

void xor_bidirectional_encode(const char* key, const size_t keyLength, char* buffer, const size_t length) {
	for (size_t i = 0; i < length; ++i) {
		buffer[i] ^= key[i % keyLength];
	}
}

void print_map(const std::map<LPVOID, DWORD>& m)
{
	for (auto it = m.cbegin(); it != m.cend(); ++it)
	{
		std::cout << "Map -> " << it->first << "[" << it->second << "]" << "\n";
	}
}

DWORD mapSize(const std::map<LPVOID, DWORD>& m)
{
	DWORD alloc = 0;
	for (auto it = m.cbegin(); it != m.cend(); ++it)
	{
		alloc += it->second;
	}

	return alloc;
}

void HeapEncryptDecryptSection(char* buffer, const size_t length) {
	xor_bidirectional_encode(key, keySize, buffer, length);
}

void HeapEncryptDecrypt() {
	SecureZeroMemory(&entry, sizeof(entry));

	while (HeapWalk(GetProcessHeap(), &entry)) {
		if ((entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) {
			HeapEncryptDecryptSection((char*)(entry.lpData), entry.cbData);
			//printf("Lengh of fragment %d\n", entry.cbData);
		}
	}
}

void HeapEncryptMap(const std::map<LPVOID, DWORD>& m) {
	for (auto it = m.cbegin(); it != m.cend(); ++it)
	{
		HeapEncryptDecryptSection((char*)(it->first), it->second);
	}
}