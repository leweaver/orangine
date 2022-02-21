#pragma once

#include <winsdkver.h>
//#define _WIN32_WINNT 0x0601
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
//#include <atlcomcli.h> // for CComPtr

// Workaround for CLION; it will try to compile a temporary file to determine compiler version. but, the temp file does
// not have a .cpp extension, which throws an error:
//#error:  WRL requires C++ compilation (use a .cpp suffix)
#ifdef __cplusplus
#include <wrl/client.h>
#endif