#include "Injector.h"

#ifdef DBGBUILD
#define CROC_INJECT_DLL_PATH		"C:\\Workspace\\CrocMod\\Debug\\CrocInject.dll"
#else 
#define CROC_INJECT_DLL_PATH		"C:\\Workspace\\CrocMod\\Release\\CrocInject.dll"
#endif

namespace {
	void alert_err(const char * const msg) {
		MessageBox(NULL, msg, "ERROR", MB_OK | MB_ICONERROR);
	}

	//http://stackoverflow.com/questions/11010165/how-to-suspend-resume-a-process-in-windows
	//Warning: UNDOCUMENTED functions!!!

	//Funcptr types need to be PRECISE; note the calling convention!
	typedef LONG(NTAPI *NtSuspendProcess)(HANDLE ProcessHandle);
	typedef LONG(NTAPI *NtResumeProcess)(HANDLE ProcessHandle);

	//Wrapper around the API
	void suspend(DWORD processId) {
		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

		//Extract the function address from ntdll
		NtSuspendProcess pfnNtSuspendProcess = (NtSuspendProcess)GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess");

		if (pfnNtSuspendProcess != NULL) { //In case the API is no longer supported
			pfnNtSuspendProcess(processHandle);
		}

		CloseHandle(processHandle);
	}

	void resume(DWORD processId)
	{
		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

		NtResumeProcess pfnNtResumeProcess = (NtResumeProcess)GetProcAddress( GetModuleHandle("ntdll"), "NtResumeProcess");

		if (pfnNtResumeProcess != NULL) { //In case the API is no longer supported
			pfnNtResumeProcess(processHandle);
		}
		CloseHandle(processHandle);
	}
}

using namespace CrocMod;

Injector::Injector(){}

Injector::~Injector()	{}

bool Injector::inject(HANDLE hCrocProc, DWORD pID) {
	//Suspend/resume the process so that no funny business happens while we toy with the machine code of the process
	suspend(pID);

	char dirPath[MAX_PATH];
	char fullPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dirPath);
	sprintf_s(fullPath, MAX_PATH, CROC_INJECT_DLL_PATH);

	LPVOID pLoadLibraryA = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

	if (pLoadLibraryA == NULL) {
		alert_err("Failed to get the address of LoadLibraryA in kernel32.dll");
		return false;
	}

	//Reserve space in the victim process for the path before writing it
	LPVOID pAllocedMem = (LPVOID)VirtualAllocEx(hCrocProc, NULL, strlen(fullPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (pAllocedMem == NULL) {
		alert_err("Failed to allocate virtual memory in the victim process");
		return false;
	}

	if (!WriteProcessMemory(hCrocProc, pAllocedMem, fullPath, strlen(fullPath), NULL)) {
		alert_err("Failed to write DLL path to allocated memory in victim process");
		return false;
	}
	
	//The most complicated but most important step: we create a thread in the process which uses LoadLibraryA as its entry point.
	//LoadLibraryA is ideal for this role, as it takes a single parameter which is easily passed by pAllocedMem (the string of the dll path).
	//Once we start the thread, it will simply execute LoadLibraryA and then exit.
	HANDLE hThread = CreateRemoteThread(hCrocProc, NULL, NULL, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pAllocedMem, NULL, NULL);

	if (hThread == NULL) {
		alert_err("Failed to create remote thread in victim process");
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hCrocProc);

	resume(pID);
}