// CrocInject.cpp : Defines the exported functions for the DLL application.
//

/*

	*****Hooking algorithms used throughout this program are adapted from http://www.codeproject.com/Articles/30140/API-Hooking-with-MS-Detours
	
*/

#include "stdafx.h"
#include "CrocInject.h"
#include <cstdio>
#include <string>

#include <glide.h> //Croc's video engine; defines the engine-specific structs which are needed as parameters for some functions

//We need forward declarations for function types which we may choose for hooking
typedef BOOL(WINAPI * pDestroyWindow)(HWND);
typedef int (WINAPI *pMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);
typedef LRESULT (WINAPI *pDefWindowProcA)(HWND, UINT, WPARAM, LPARAM);
typedef void (WINAPI *pGrGlideInit)(void);
typedef void (WINAPI *pGrBufferSwap)(int);

int WINAPI MyMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
BOOL WINAPI myDestroyWindow(HWND);
LRESULT WINAPI myDefWindowProcA(HWND, UINT, WPARAM, LPARAM);
void WINAPI myGrGlideInit(void);
void WINAPI myGrBufferSwap(int);

const size_t machineCodeSize = 6;

void BeginRedirect(LPVOID);

void *pOrigFuncAddr = NULL;

BYTE oldBytes[machineCodeSize] = { 0 };
BYTE JMP[machineCodeSize] = { 0 };
DWORD oldProtect, myProtect = PAGE_EXECUTE_READWRITE;

void BeginRedirect(LPVOID newFunction) {

	//Our hooking code: equivalent to the following x86 assembly:
	//JMP RELATIVE <garbage addr>
	//RET <- decrease the likelihood of a crash if our JMP somehow fails
	BYTE tempJMP[machineCodeSize] = { 0xE9, 0x90, 0x90, 0x90, 0x90, 0xC3 };
	memcpy(JMP, tempJMP, machineCodeSize);

	//Calculate the offset amount for our relative jump
	DWORD JMPSize = ((DWORD)newFunction - (DWORD)pOrigFuncAddr - 5); //-5 accounts for the first 5 bytes of our injectable which have been read before the jmp takes place

	//Make sure the OS cooperates and we don't trigger a segmentation fault when we alter the machine code
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize,
		PAGE_EXECUTE_READWRITE, &oldProtect); //Note we save the original page attrs

	//Save the original 6 bytes which we will be overriding
	memcpy(oldBytes, pOrigFuncAddr, machineCodeSize);

	//Place the offset amount into our injectable machine code. The machine code is now equivalent to the following assembly:
	//JMP RELATIVE <distance between the current EIP and the address of our injectable code>
	//RET
	memcpy(&JMP[1], &JMPSize, 4);

	//Override the first 6 bytes of our target function with our now-complete machine code
	memcpy(pOrigFuncAddr, JMP, machineCodeSize);

	//Reset page attrs
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, oldProtect, NULL);
}

/*

	In general, our injected function does the following:
		-	Calls the original function by rewriting the original machine code at our hooked function. By rewriting the machine code,
				we can invoke the function within our hooked function and in turn call the original version of the function. After
				saving the return value from the call, we put our injectable machine code back in. It's complicated... check out the 
				hooked version of DefWindowProcA below.
		- Either before or after the call to the original function, we are free to do anything we want, including calling any function
				addressable by the program.
		-	We return the value from the call to the original function or, if we did not call the original function, we make sure to 
				return a value that the program expects, otherwise we may get a crash. Note also that not calling the original function 
				can cause the program to crash for various reasons, as would be the case if the function modified some global state variable
				that affects the program's control flow.

*/

int  WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uiType) {
	//MessageBox(NULL, "HOOKING SUCCESS", "YAY!", MB_OK)

	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, myProtect, NULL);
	memcpy(pOrigFuncAddr, oldBytes, machineCodeSize);
	int retValue = MessageBoxW(NULL, L"HOOKING SUCCESS!!!", L"Success:", MB_OK | MB_ICONQUESTION);
	memcpy(pOrigFuncAddr, JMP, machineCodeSize);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, oldProtect, NULL);
	return retValue;
}

BOOL WINAPI myDestroyWindow(HWND hwnd) {
	MessageBox(NULL, "HOOKING SUCCESS", "YAY!", MB_OK);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, myProtect, NULL);
	memcpy(pOrigFuncAddr, oldBytes, machineCodeSize);
	BOOL retValue = DestroyWindow(hwnd);
	memcpy(pOrigFuncAddr, JMP, machineCodeSize);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, oldProtect, NULL);
	return retValue;
}

LRESULT WINAPI myDefWindowProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	pDefWindowProcA origProc = (pDefWindowProcA)pOrigFuncAddr;

	//Do our custom code: beep synchronously when the 'b' key is pressed.
	if (msg == WM_KEYDOWN && wParam == 'B') {
		Beep(440, 250);
	}

	//Set page attrs for safety
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, myProtect, NULL);

	//Rewrite the original machine code which we had replaced; i.e. revert the program back to its original form temporarily
	memcpy(pOrigFuncAddr, oldBytes, machineCodeSize);

	//Save the return value, and call the original function (restored with the original machine code) with the parameters that were intended for it
	BOOL retValue = origProc(hwnd, msg, wParam, lParam);

	//Rewrite our injectable machine code
	memcpy(pOrigFuncAddr, JMP, machineCodeSize);

	//Reset page attrs
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, oldProtect, NULL);

	//Give the program a return value that it expects
	return retValue;
}

void WINAPI myGrGlideInit(void) {
	MessageBox(NULL, "HERE", "HERE", MB_OK | MB_SYSTEMMODAL);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, myProtect, NULL);
	memcpy(pOrigFuncAddr, oldBytes, machineCodeSize);
	pGrGlideInit origFunc = (pGrGlideInit)pOrigFuncAddr;
	origFunc();
	memcpy(pOrigFuncAddr, JMP, machineCodeSize);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, oldProtect, NULL);
	return;
}

void WINAPI myGrBufferSwap(int paramA) {
	pGrBufferSwap pOrigFunc = (pGrBufferSwap)pOrigFuncAddr;

	MessageBox(NULL, "HERE", "HERE", MB_OK | MB_SYSTEMMODAL);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, myProtect, NULL);
	memcpy(pOrigFuncAddr, oldBytes, machineCodeSize);
	pOrigFunc(paramA);
	memcpy(pOrigFuncAddr, JMP, machineCodeSize);
	VirtualProtect((LPVOID)pOrigFuncAddr, machineCodeSize, oldProtect, NULL);
	return;
}


int CrocMod::init() {
	return 1;
}

void CrocMod::my_func() {

	//Save the address of the hooked function
	pOrigFuncAddr = (pDefWindowProcA)GetProcAddress(GetModuleHandle("user32.dll"), "DefWindowProcA");

	//Stop if the function could not be found
	if (pOrigFuncAddr != NULL) {
		char buff[256];
		sprintf_s(buff, "Hook set successfully at 0x%X", pOrigFuncAddr);
		MessageBox(NULL, buff, "Success!", MB_OK | MB_ICONINFORMATION);
		BeginRedirect(myDefWindowProcA);
	} else {
		MessageBox(NULL, "Failed to set hook!", "Error", MB_OK | MB_ICONERROR);
	}
}

void CrocMod::my_func_close() {
	//Remove our injected code (Not called in this implementation
	memcpy(pOrigFuncAddr, oldBytes, machineCodeSize);
}


