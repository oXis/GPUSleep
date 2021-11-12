#pragma once
#include <windows.h>
#include <tlhelp32.h>

// Pass 0 as the targetProcessId to suspend threads in the current process
void DoSuspendThreads(DWORD targetProcessId, DWORD targetThreadId)
{
	printf("Own thread %d\n", targetThreadId);
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(h, &te))
		{
			do
			{
				if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
				{
					// Suspend all threads EXCEPT the one we want to keep running
					if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
					{
						HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
						if (thread != NULL)
						{
							printf("Suspending thread %d\n", te.th32ThreadID);
							SuspendThread(thread);
							CloseHandle(thread);
						}
					}
				}
				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));
		}
		CloseHandle(h);
	}
}

void DoResumeThreads(DWORD targetProcessId, DWORD targetThreadId)
{
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(h, &te))
		{
			do
			{
				if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
				{
					// Suspend all threads EXCEPT the one we want to keep running
					if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
					{
						HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
						if (thread != NULL)
						{
							printf("Resuming thread %d\n", te.th32ThreadID);
							ResumeThread(thread);
							CloseHandle(thread);
						}
					}
				}
				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));
		}
		CloseHandle(h);
	}
}