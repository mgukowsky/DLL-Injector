// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "CrocInject.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//This case is executed once the process loads the DLL
		MessageBox(NULL, "DLL loaded by Dllmain!", "Success!", MB_OK);
		CrocMod::my_func();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		/*
			Many sources say to clean up the DLL patch upon the DLL being unloaded, assuming that this occurs when the 
			hooked process exits. At least on my system, however, the DLL is always detached very quickly after it is attached, 
			which causes our patch to be undone before it can have any effect. Maybe this is a Windows 10 thing with how DLLs
			are loaded? In any case, I chose not to call the cleanup code, which should be OK since the CrocInject DLL is not used
			by any other programs.

			***The main reason not to call the cleanup callback is because we want our patch to persist throughout the program's lifetime
		*/
		/*char buff[128];
		sprintf_s(buff, "Closing DLL with *lpReserved == %d", *((DWORD *)lpReserved));
		MessageBox(NULL, buff, "MSG", MB_OK);*/
		//CrocMod::my_func_close();
		break;
	}
	return TRUE;
}

