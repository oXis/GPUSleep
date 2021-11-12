#pragma once

#include <windows.h>
#include "cuda.h"

HMODULE MoveDLLToStrorage(HMODULE dll);
VOID MoveDLLFromStrorage(HMODULE dll, HMODULE storage);

ULONG_PTR MoveDLLToGPUStrorage(HMODULE dll, PDWORD SizeOfHeaders, PNVIDIA_API_TABLE Api);
VOID MoveDLLFromGPUStrorage(HMODULE dll, ULONG_PTR storage, DWORD SizeOfHeaders, PNVIDIA_API_TABLE Api);
