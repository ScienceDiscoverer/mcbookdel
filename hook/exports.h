#pragma once

#include <windows.h>

#define EXP extern "C" __declspec(dllexport)




EXP LRESULT CALLBACK llKbHookProc(int code, WPARAM wp, LPARAM lp);


/*___________________________________________________________________
|  setMasterThreadId:			 
|    Initialises shared DLL memory with host program's thread ID
|  
|  m_thread_id: Host thread ID
|       mb_msg: Registered Windows User mess. for mouse mbutton
|		mw_msg: Registered Windows User mess. for mouse wheel
|        k_msg: Registered Windows User mess. for keyboad events
|
|  Return value:
|        Host thread ID not set -> 0
|    Host thread ID already set -> Current host thread ID
|____________________________________________________________________*/
EXP DWORD setMasterThreadId(DWORD m_thread_id, UINT k_msg);