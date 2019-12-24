//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <WinSDKVer.h>
// #define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>

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

#include <wrl/client.h>

#include <d3d11_1.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <DirectXColors.h>
#include <DirectXHelpers.h>
#include <DirectXMath.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>

#include <cstdio>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <g3log/g3log.hpp>
