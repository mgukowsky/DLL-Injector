#include "main.h"

/*

*****Hooking algorithms used throughout this program are adapted from http://www.codeproject.com/Articles/30140/API-Hooking-with-MS-Detours

*/

//Change these strings according to your install configuration
const char * const	CROCPATH = "C:/Users/mguko_000/Desktop/CROC/INSTALLATION/croc.exe";
const char * const	CROCDIR = "C:/Users/mguko_000/Desktop/CROC/INSTALLATION";

int main(int argc, char **argv) {
	//Win32 boilerplate
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	SECURITY_ATTRIBUTES mySecAttrs = { sizeof(SECURITY_ATTRIBUTES), NULL, true };

	int processResult = CreateProcess(CROCPATH, NULL, NULL, NULL, false,
		DETACHED_PROCESS, NULL, CROCDIR, &si, &pi);

	CrocMod::Injector i;
	if (!i.inject(pi.hProcess, pi.dwProcessId))
		return 1;

	return 0;
}