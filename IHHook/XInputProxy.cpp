// XInputProxy.cpp - proxy for xinput1_3.dll
#include "windowsapi.h"
#include <stdlib.h>
#include <stdio.h>
#include <filesystem>


#include "spdlog/spdlog.h"

#pragma comment(linker, "/export:XInputGetState=XInputGetState,@2")
#pragma comment(linker, "/export:XInputSetState=XInputSetState,@3")
#pragma comment(linker, "/export:XInputGetCapabilities=XInputGetCapabilities,@4")
#pragma comment(linker, "/export:XInputEnable=XInputEnable,@5")
#pragma comment(linker, "/export:XInputGetDSoundAudioDeviceGuids=XInputGetDSoundAudioDeviceGuids,@6")
#pragma comment(linker, "/export:XInputGetBatteryInformation=XInputGetBatteryInformation,@7")
#pragma comment(linker, "/export:XInputGetKeystroke=XInputGetKeystroke,@8")

// Undocumented ordinals
#pragma comment(linker, "/export:ordinal100=ordinal100,@100,NONAME")
#pragma comment(linker, "/export:ordinal101=ordinal101,@101,NONAME")
#pragma comment(linker, "/export:ordinal102=ordinal102,@102,NONAME")
#pragma comment(linker, "/export:ordinal103=ordinal103,@103,NONAME")

//structs
struct XINPUT_GAMEPAD;
struct XINPUT_STATE;
struct XINPUT_VIBRATION;
struct XINPUT_CAPABILITIES;
struct XINPUT_BATTERY_INFORMATION;
struct XINPUT_KEYSTROKE;
typedef XINPUT_KEYSTROKE* PXINPUT_KEYSTROKE;

typedef DWORD(WINAPI* XInputGetState_ptr)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD(WINAPI* XInputSetState_ptr)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
typedef DWORD(WINAPI* XInputGetCapabilities_ptr)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
typedef void(WINAPI* XInputEnable_ptr)(BOOL enable);
typedef DWORD(WINAPI* XInputGetDSoundAudioDeviceGuids_ptr)(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid);
typedef DWORD(WINAPI* XInputGetBatteryInformation_ptr)(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation);
typedef DWORD(WINAPI* XInputGetKeystroke_ptr)(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke);

// Ordinal 100 is XInputGetStateEx 
typedef DWORD(WINAPI* XInputGetStateEx_ptr)(DWORD dwUserIndex, void* pState); 

XInputGetState_ptr               XInputGetState_Orig = nullptr;
XInputSetState_ptr               XInputSetState_Orig = nullptr;
XInputGetCapabilities_ptr        XInputGetCapabilities_Orig = nullptr;
XInputEnable_ptr                 XInputEnable_Orig = nullptr;
XInputGetDSoundAudioDeviceGuids_ptr XInputGetDSoundAudioDeviceGuids_Orig = nullptr;
XInputGetBatteryInformation_ptr  XInputGetBatteryInformation_Orig = nullptr;
XInputGetKeystroke_ptr           XInputGetKeystroke_Orig = nullptr;
XInputGetStateEx_ptr XInputGetStateEx_Orig = nullptr;

FARPROC ordinal101_Orig = nullptr;
FARPROC ordinal102_Orig = nullptr;
FARPROC ordinal103_Orig = nullptr;

extern HMODULE g_thisModule;
HMODULE origDll = nullptr;
static std::once_flag s_xinputLoadFlag{};


void LoadProxiedDll() {

	std::call_once(s_xinputLoadFlag, []() -> void {

		// System directory
		WCHAR systemDirBuf[MAX_PATH]{};
		if (!GetSystemDirectoryW(systemDirBuf, _countof(systemDirBuf)))
		{
			spdlog::error("GetSystemDirectoryW failed");
			return;
		}
		const std::filesystem::path systemDir = systemDirBuf;

		// Full path of this (proxy) DLL
		WCHAR ourModulePathBuf[MAX_PATH]{};
		if (!GetModuleFileNameW(g_thisModule, ourModulePathBuf, _countof(ourModulePathBuf)))
		{
			spdlog::error("GetModuleFileNameW failed");
			return;
		}
		const std::filesystem::path ourModulePath = ourModulePathBuf;

		// Same filename as the original system DLL
		const std::filesystem::path modulePath = systemDir / ourModulePath.filename();

		spdlog::debug("Loading original module from: {}", modulePath.string());

		origDll = LoadLibraryW(modulePath.c_str());
		if (!origDll)
		{
			spdlog::error("Could not load original module (error {})", GetLastError());
			return;
		}

		XInputGetState_Orig = (XInputGetState_ptr)GetProcAddress(origDll, "XInputGetState");
		XInputSetState_Orig = (XInputSetState_ptr)GetProcAddress(origDll, "XInputSetState");
		XInputGetCapabilities_Orig = (XInputGetCapabilities_ptr)GetProcAddress(origDll, "XInputGetCapabilities");
		XInputEnable_Orig = (XInputEnable_ptr)GetProcAddress(origDll, "XInputEnable");
		XInputGetDSoundAudioDeviceGuids_Orig = (XInputGetDSoundAudioDeviceGuids_ptr)GetProcAddress(origDll, "XInputGetDSoundAudioDeviceGuids");
		XInputGetBatteryInformation_Orig = (XInputGetBatteryInformation_ptr)GetProcAddress(origDll, "XInputGetBatteryInformation");
		XInputGetKeystroke_Orig = (XInputGetKeystroke_ptr)GetProcAddress(origDll, "XInputGetKeystroke");

		// Undocumented ordinals
		XInputGetStateEx_Orig = (XInputGetStateEx_ptr)GetProcAddress(origDll, (LPCSTR)100);
		ordinal101_Orig = GetProcAddress(origDll, (LPCSTR)101);
		ordinal102_Orig = GetProcAddress(origDll, (LPCSTR)102);
		ordinal103_Orig = GetProcAddress(origDll, (LPCSTR)103);
		
		});
}

extern "C" __declspec(dllexport) DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	if (!XInputGetState_Orig)
		LoadProxiedDll();

	return XInputGetState_Orig(dwUserIndex, pState);
}

extern "C" __declspec(dllexport) DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
	if (!XInputSetState_Orig)
		LoadProxiedDll();

	return XInputSetState_Orig(dwUserIndex, pVibration);
}

extern "C" __declspec(dllexport) DWORD WINAPI XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities)
{
	if (!XInputGetCapabilities_Orig)
		LoadProxiedDll();

	return XInputGetCapabilities_Orig(dwUserIndex, dwFlags, pCapabilities);
}

extern "C" __declspec(dllexport) void WINAPI XInputEnable(BOOL enable)
{
	if (!XInputEnable_Orig)
		LoadProxiedDll();

	XInputEnable_Orig(enable);
}

extern "C" __declspec(dllexport) DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid)
{
	if (!XInputGetDSoundAudioDeviceGuids_Orig)
		LoadProxiedDll();

	return XInputGetDSoundAudioDeviceGuids_Orig(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);
}

extern "C" __declspec(dllexport) DWORD WINAPI XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation)
{
	if (!XInputGetBatteryInformation_Orig)
		LoadProxiedDll();

	return XInputGetBatteryInformation_Orig(dwUserIndex, devType, pBatteryInformation);
}

extern "C" __declspec(dllexport) DWORD WINAPI XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke)
{
	if (!XInputGetKeystroke_Orig)
		LoadProxiedDll();

	return XInputGetKeystroke_Orig(dwUserIndex, dwReserved, pKeystroke);
}

// forward this just in case
extern "C" __declspec(dllexport) DWORD WINAPI ordinal100(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	if (!XInputGetStateEx_Orig)
		LoadProxiedDll();

	return XInputGetStateEx_Orig(dwUserIndex, pState);
}
extern "C" __declspec(dllexport) void __stdcall ordinal101() { if (!ordinal101_Orig) LoadProxiedDll(); ordinal101_Orig(); }
extern "C" __declspec(dllexport) void __stdcall ordinal102() { if (!ordinal102_Orig) LoadProxiedDll(); ordinal102_Orig(); }
extern "C" __declspec(dllexport) void __stdcall ordinal103() { if (!ordinal103_Orig) LoadProxiedDll(); ordinal103_Orig(); }