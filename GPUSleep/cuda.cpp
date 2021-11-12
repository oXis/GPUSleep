#include <Windows.h>
#include <iostream>
#include <stdio.h>

#include "cuda.h"
#include "utils.h"
#pragma warning(disable:6011)

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

BOOL InitNvidiaCudaAPITable(PNVIDIA_API_TABLE Api)
{

	if (Api->CudaInit) {
		return TRUE;
	}

	Api->NvidiaLibary = LoadLibraryW(L"nvcuda.dll");
	if (Api->NvidiaLibary == NULL)
		return FALSE;

	Api->CudaCreateContext = (CUDACREATECONTEXT)GetProcAddress(Api->NvidiaLibary, "cuCtxCreate_v2");
	Api->CudaGetDevice = (CUDAGETDEVICE)GetProcAddress(Api->NvidiaLibary, "cuDeviceGet");
	Api->CudaGetDeviceCount = (CUDAGETDEVICECOUNT)GetProcAddress(Api->NvidiaLibary, "cuDeviceGetCount");
	Api->CudaInit = (CUDAINIT)GetProcAddress(Api->NvidiaLibary, "cuInit");
	Api->CudaMemoryAllocate = (CUDAMEMORYALLOCATE)GetProcAddress(Api->NvidiaLibary, "cuMemAlloc_v2");
	Api->CudaMemoryCopyToDevice = (CUDAMEMORYCOPYTODEVICE)GetProcAddress(Api->NvidiaLibary, "cuMemcpyHtoD_v2");
	Api->CudaMemoryCopyToHost = (CUDAMEMORYCOPYTOHOST)GetProcAddress(Api->NvidiaLibary, "cuMemcpyDtoH_v2");
	Api->CudaMemoryFree = (CUDAMEMORYFREE)GetProcAddress(Api->NvidiaLibary, "cuMemFree_v2");
	Api->CudaDestroyContext = (CUDADESTROYCONTEXT)GetProcAddress(Api->NvidiaLibary, "cuCtxDestroy");

	if (!Api->CudaCreateContext || !Api->CudaGetDevice || !Api->CudaGetDeviceCount || !Api->CudaInit || !Api->CudaDestroyContext)
		return FALSE;

	if (!Api->CudaMemoryAllocate || !Api->CudaMemoryCopyToDevice || !Api->CudaMemoryCopyToHost || !Api->CudaMemoryFree)
		return FALSE;

	return TRUE;
}

ULONG_PTR RtlAllocateGpuMemory(PNVIDIA_API_TABLE Api, DWORD ByteSize)
{
	ULONG_PTR GpuBufferPointer = NULL;

	if (ByteSize == 0)
		return NULL;

	if (Api->CudaMemoryAllocate((ULONG_PTR)&GpuBufferPointer, ByteSize) != CUDA_SUCCESS) {
		return NULL;
	}

	return GpuBufferPointer;

}

CUDA_CONTEXT initCuda(NVIDIA_API_TABLE* Api, CUDA_CONTEXT* ctx) {

	INT DeviceCount = 0;
	INT Device = 0;

	if (!InitNvidiaCudaAPITable(Api))
		return NULL;

	if (Api->CudaInit(0) != CUDA_SUCCESS)
		return NULL;

	if (Api->CudaGetDeviceCount(&DeviceCount) != CUDA_SUCCESS || DeviceCount == 0)
		return NULL;

	if (Api->CudaGetDevice(&Device, DeviceCount - 1) != CUDA_SUCCESS)
		return NULL;

	if (Api->CudaCreateContext(ctx, 0, Device) != CUDA_SUCCESS)
		return NULL;

	return Context;
}