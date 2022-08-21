#include <windows.h>
#include "gui.h"
#include "data.h"
#include "keys.h"
#include "stopwatch.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef UNICODE
#define TOWSTR(x) L ## x
#else
#define TOWSTR(x) x
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_CALL(x) if((x) != S_OK) \
	{ MessageBox(NULL, TOWSTR(#x) ## " line: " TOSTRING(__LINE__), \
	TOWSTR("mcbookdel error"), MB_OK); return -1; }

#define APP_GUID "-{1EAD98DF-9633-4641-9197-8AAD76026F50}"
#define REGISTER_MESSAGE(n) \
	static const UINT n = RegisterWindowMessage(TOWSTR(#n APP_GUID))

// Supported hook types - -
#define HOOK_TASKBAR_MB 0x1
#define HOOK_GLOBAL_KB  0x2
// - - - - - - - - - - - -


typedef DWORD(__stdcall* MASTERPROC)(DWORD master_thread_id, UINT k_msg);

REGISTER_MESSAGE(UWM_KEYPRESS);    // WP: Key State   | LP: LW: Virt.C. HW: Scan C.
REGISTER_MESSAGE(UWM_NEWINSTANCE); // WP: Not Used    | LP: Not Used
REGISTER_MESSAGE(UWM_GUI_ACTION);  // WP: GUI_ACTION  | LP: New Value

// Config variables - -
bool autostart = false;
// - - - - - - - - - -

// Config variable names - - - - - -
LPCWSTR keyb_hook_n = L"keyb_hook";
LPCWSTR block_inp_n = L"block_inp";
LPCWSTR key_virtc_n = L"key_virtc";
LPCWSTR key_scanc_n = L"key_scanc";
// - - - - - - - - - - - - - - - - -

bool window_is_alive;
bool filter_is_setting_up; // User is choosing global key

HINSTANCE dll_start_addr; // A.K.A. "The Instance"
HHOOK mouse_hook;
HHOOK ll_kb_hook;

int respondToKeypress(WPARAM k_msg, LPARAM key);

/*___________________________________________________________________
|  respondToGUIaction:
|    Handles any possible GUI action taken
|
|   a_type: One of GUI_ACTION codes
|  new_val: New value of the global config passed by GUI
|
|  Return value:
|    Action handled successfully -> 0
|           Something went worng -> -1
|____________________________________________________________________*/
int respondToGUIaction(int a_type, int new_val);


/*___________________________________________________________________
|  startHooking:
|    Sets up Windows Hooks specified by input parameter
|
|  whats_hooking: One or multiple s. HOOK_ types (use | to combine)
|
|  Return value:
|    Hook(s) hooked -> 0
|    Hook(s) missed -> -1
|____________________________________________________________________*/
int startHooking(int whats_hooking);


/*___________________________________________________________________
|  stopHooking:
|    Unhooks one or more of the already running Windows Hooks
|
|  whats_unhooking: One or multiple s. HOOK_ types (use | to combine)
|
|  Return value:
|    Hook(s) unhooked -> 0
|       Hook(s) stuck -> -1
|____________________________________________________________________*/
int stopHooking(int whats_unhooking);






// HINSTANCE -> "handle" to "instance" aka "module".
// It's NOT a handle. And not to "instance" or "module".
// I's all 16 bit Windows legacy backwards compatability nonsense.
// Since 16-bit Windows had a common address space, the GetInstanceData function was really
// nothing more than a hmemcpy, and many programs relied on this and just used raw hmemcpy
// instead of using the documented API.
// In Win32/Win64 it's actually executable (DLL or EXE) image.
// HINSTANCE points to actual virtual adress where first byte of
// executable's code is located: cout << (const char*)hinst ---> MZ? (? = 0x90/0x00)
int WINAPI wWinMain(
	_In_		HINSTANCE hinst,	// "Handle" to "instance"
	_In_opt_	HINSTANCE phinst,	// "Handle" to "previous instance", always NULL
	_In_		PWSTR cmd,			// Command line arguments
	_In_		int show)			// Default user preference for ShowWindow()
{
	// Seems that only local thread spesific hooks require this
	// I will still add other custom messages, just in case!
	ChangeWindowMessageFilter(UWM_KEYPRESS, MSGFLT_ADD);
	ChangeWindowMessageFilter(UWM_NEWINSTANCE, MSGFLT_ADD);
	ChangeWindowMessageFilter(UWM_GUI_ACTION, MSGFLT_ADD);


#ifndef NINJA
	AllocConsole();
	FILE* s = freopen("CONIN$", "r", stdin);
	s = freopen("CONOUT$", "w", stdout);
	s = freopen("CONOUT$", "w", stderr);
#endif

	PLH("HINST: " << hinst);

	// Check user settings and init GUI
	//autostart = regAutoStChk();
	autostart = taskAutoStChk();

	setControls(autostart);

	if(startHooking(HOOK_GLOBAL_KB) != 0)
	{
		return -1;
	}

//#ifdef NINJA
	if(wcscmp(cmd, L"-silent"))
	{
		initGUI(hinst, UWM_GUI_ACTION);
		CHECK_CALL(!spawnMainWnd());
		window_is_alive = true;
	}
//#endif

	ULONGLONG stime = 0, etime = 0;
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if(msg.message == UWM_NEWINSTANCE)
		{
			if(!window_is_alive)
			{
				initGUI(hinst, UWM_GUI_ACTION);
				CHECK_CALL(!spawnMainWnd());
				window_is_alive = true;
			}
		}
		else if(msg.message == UWM_KEYPRESS)
		{
			respondToKeypress(msg.wParam, msg.lParam);
		}
		else if(msg.message == UWM_GUI_ACTION)
		{
			respondToGUIaction((int)msg.wParam, (int)msg.lParam);
		}

		// Only needed when Edit input boxes are used
		//TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	stopHooking(HOOK_TASKBAR_MB | HOOK_GLOBAL_KB);

	return (int)msg.wParam;
}

void prsdkb(bool down)
{
	INPUT inp;

	inp.type = INPUT_KEYBOARD;

	inp.ki.wVk = VK_DELETE;
	inp.ki.wScan = 0x53;
	inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	inp.ki.time = 0x0;
	inp.ki.dwExtraInfo = 0x0;

	inp.ki.dwFlags |= down ? 0x0 : KEYEVENTF_KEYUP;

	SendInput(1, &inp, sizeof(INPUT));
}

void prsmmb(bool down)
{
	INPUT inp;

	inp.type = INPUT_MOUSE;

	inp.mi.dx = 0x0;
	inp.mi.dy = 0x0;
	inp.mi.mouseData = 0x0;
	inp.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
	inp.mi.time = 0x0;
	inp.mi.dwExtraInfo = 0x0;

	SendInput(1, &inp, sizeof(INPUT));
}

int respondToKeypress(WPARAM k_msg, LPARAM key)
{
	bool kdown = k_msg == WM_KEYDOWN || k_msg == WM_SYSKEYDOWN;


	static SdStopwatch sw;


	//static SdStopwatch long_prs;
	static SdStopwatch dbl_prs;
	static bool dbprs = false;
	static int kdowns = 0;

	switch(key)
	{
	case K_ROPTION:
		prsdkb(kdown);
		break;
	case K_RCOMMAND:
		if(kdown)
		{
			if(++kdowns == 1)
			{
				prsmmb(DOWN);
			}
		}
		else
		{
			kdowns = 0;
			prsmmb(UP);
		}
		
		break;
	default:
		return -1;
	}
	
	return 0;
}

int respondToGUIaction(int a_type, int new_val)
{
	switch(a_type)
	{
	case GUI_AUTOSTART_CHBX:
		if(new_val)
		{
			autostart = true;
			//regAutoStSet(true);
			taskAutoStSet(true);
		}
		else
		{
			autostart = false;
			//regAutoStSet(false);
			taskAutoStSet(false);
		}
		break;
	case GUI_GLOB_KEYB_CHBX:
		break;
	case GUI_BLOCK_INP_CHBX:
		break;
	case GUI_SAVE_KEYC_BUTT:
	case GUI_TERMINATE_BUTT:
		PostQuitMessage(0);
		break;
	case GUI_WINDOWWAS_DEST:
		window_is_alive = false;
		break;
	default:
		return -1;
	}

	return 0;
}

int startHooking(int whats_hooking)
{
	bool first = true; // First hook setup after launch of the instance
	if(dll_start_addr != NULL)
	{
		first = false;
	}
	
	// HACKERMAN MODE: ON
	// Obtain taskbar thread_id by class name
	HWND taskb_hwnd = FindWindow(L"Shell_TrayWnd", NULL);
	DWORD proc_id = NULL;
	DWORD thread_id = GetWindowThreadProcessId(taskb_hwnd, &proc_id);

	// Load hooking DLL, get function pointers
	MASTERPROC setMasterThreadId = NULL;
	HOOKPROC mouseHookCback = NULL;
	HOOKPROC llKbHookCback = NULL;

	if(first)
	{
		dll_start_addr = LoadLibrary(L"hook");
	}

	if(dll_start_addr != NULL)
	{
		setMasterThreadId = (MASTERPROC)GetProcAddress(dll_start_addr, "setMasterThreadId");
		llKbHookCback = (HOOKPROC)GetProcAddress(dll_start_addr, "llKbHookProc");
	}
	else
	{
		MessageBox(NULL, L"GetProcAddress(dll_inst, \"mouseHookProc\") line: 65", L"mcbookdel error", MB_OK);
		return -1;
	}

	if(llKbHookCback != NULL)
	{
		// Will this multi-instance check work with shared memory? No idea (._. )
		// YES, IT ACTUALLY DOES! ( o_0 )
		DWORD res = setMasterThreadId(GetCurrentThreadId(), UWM_KEYPRESS);
		if(first && res) // Only check for multi-instance at first hooking
		{
			//MessageBox(NULL, L"2NDINSTANCE!!", L"mcbookdel error", MB_OK);
			PostThreadMessage(res, UWM_NEWINSTANCE, 0, 0);
			return -1;
		}

		/*if(whats_hooking & HOOK_TASKBAR_MB)
		{
			mouse_hook = SetWindowsHookEx(WH_MOUSE, mouseHookCback, dll_start_addr, thread_id);
		}*/
		// Oh boi, here it goes... The real hackin begins! Hook em' up, ladies'!
		if(whats_hooking & HOOK_GLOBAL_KB)
		{
			ll_kb_hook = SetWindowsHookEx(WH_KEYBOARD_LL, llKbHookCback, dll_start_addr, 0);
			PLH("LL_KB_HOOK: " << ll_kb_hook);
		}
		// But will the global hook work?..
		// Yes it does!
	}
	else
	{
		MessageBox(NULL, L"SetWindowsHookEx", L"mcbookdel error", MB_OK);
		return -1;
	}

	return 0;
}

int stopHooking(int whats_unhooking)
{
	bool res = true;
	if(whats_unhooking & HOOK_TASKBAR_MB)
	{
		res = (bool)UnhookWindowsHookEx(mouse_hook);
	}
	if(whats_unhooking & HOOK_GLOBAL_KB)
	{
		res = (bool)UnhookWindowsHookEx(ll_kb_hook);
	}

	return !res;
}

// Double click and signle click on same button results in horrible
// delay for single click. This is sad.
//DWORD WINAPI delKeyThread(LPVOID dbprs)
//{
//	PLH("DEL THREAD UP");
//	Sleep(150);
//	if(*(bool*)dbprs)
//	{
//		PLH("DEL THREAD NO PRESS");
//		return 0;
//	}
//	else
//	{
//		PLH("DEL THREAD PRESSIN DEL");
//		prskb(VK_DELETE, DOWN);
//		return 0;
//	}
//
//	if(kdown)
//	{
//		//PLH("BACKSLASH DOWN");
//		dbl_prs.Stop();
//		if(dbprs || dbl_prs.Ms() < 70)
//		{
//			PLH("BACKSLASH DPRS " << dbl_prs.Str());
//			dbprs = true;
//			sw.Stop();
//			PLD("TIME AFTER THREAD SPAWNED: " << sw.Str());
//			PLD("DBPRS IS NOW TRUE!");
//			prskb(VK_OEM_5, DOWN); // '\|' KEY
//		}
//		else
//		{
//			PLH("BACKSLASH SPRS " << dbl_prs.Str());
//			CreateThread(NULL, 0, delKeyThread, &dbprs, 0, NULL);
//			sw.Set();
//		}
//	}
//	else
//	{
//		PLH("BACKSLASH UP");
//		dbl_prs.Set();
//		if(dbprs)
//		{
//			prskb(VK_OEM_5, UP);
//		}
//		else
//		{
//			prskb(VK_DELETE, UP);
//		}
//		dbprs = false;
//		PLD("DBPRS IS NOW FALSE!");
//	}
//}