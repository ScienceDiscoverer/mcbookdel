#pragma once

typedef unsigned long long ui64;

//#define LDW 
//#define HDW
#define DOWN true
#define UP   false

// All keys must have scan code in low DWORD and virtual code in high DWORD
// Apple MacBook Air A1466
#define K_BACKSLASH 0xDC0000002B // VC0xDC SC0x2B Backslash on MacBook's keyboard, under "delete"
#define K_ROPTION   0xA500000038 // VC0xA5 SC0x38 Right "Option" button (RAlt key)
#define K_RCOMMAND  0x5C0000005C // VC0x5C SC0x5C Right "Command" button (RWindows key) 


#define NINJA

#ifndef NINJA
#define PLH(x) std::cout << std::hex << std::uppercase << x << std::endl
#define PLD(x) std::cout << std::dec << x << std::endl
#define WPLH(x) std::wcout << std::hex << std::uppercase << x << std::endl
#define WPLD(x) std::wcout << std::dec << x << std::endl
#define PERR perr()
#else
#define PLH(x)
#define PLD(x)
#define WPLH(x)
#define WPLD(x)
#define PERR
#endif