#include "exports.h"
#include "..\mcbookdel\keys.h"

#define IGNORE_FILTER 0xFFFFFFFF

#pragma data_seg(".shared")
DWORD master_thread_id = NULL;
UINT UWM_KEYPRESS = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,RWS")

using namespace std;

BOOL APIENTRY DllMain(
	_In_	HMODULE hm,			// "Handle" to "Module" in fact its base adress of DLL
	_In_	DWORD creason,		// Reason for calling this function by the OS
	_In_	LPVOID reserved)	// Dynamic/Statc link flag or FreeLibrary/Process term.
{
	switch(creason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hm);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

LRESULT CALLBACK llKbHookProc(int code, WPARAM wp, LPARAM lp)
{
	switch(code)
	{
	case HC_ACTION:
	{
		KBDLLHOOKSTRUCT* khs = (KBDLLHOOKSTRUCT*)lp;

		ui64 k = ((ui64)khs->vkCode << 32) | (ui64)khs->scanCode;
		
		switch(k)
		{
		case K_ROPTION:
		case K_RCOMMAND:
			PostThreadMessage(master_thread_id, UWM_KEYPRESS, wp, k);
			return -1;
		default:
			return CallNextHookEx(NULL, code, wp, lp);
		}
		break;
	}
	default:
		break;
	}

	return CallNextHookEx(NULL, code, wp, lp);
}

DWORD setMasterThreadId(DWORD m_thread_id, UINT k_msg)
{
	if(master_thread_id != NULL)
	{
		return master_thread_id;
	}
	
	master_thread_id = m_thread_id;
	UWM_KEYPRESS = k_msg;
	return 0;
}