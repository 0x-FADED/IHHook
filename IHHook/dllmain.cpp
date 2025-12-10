// dllmain.cpp
// dll entry
// See IHHook.h for comments on rough layout of parts of the project

#include <filesystem>

#include "Hooks_FOV.h" //DEBUGNOW
#include "IHHook.h"
#include "windowsapi.h"

HMODULE g_thisModule;
extern HMODULE origDll; // dinputproxy

static DWORD WINAPI InitThread(LPVOID lpParameter)
{
    if (g_ihhook->isD3D11Loaded == true)
    {
        g_ihhook->Initialize();
    }

    return 0;
} //InitThread

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        g_thisModule = hModule;

        g_ihhook = std::make_unique<IHHook::IHH>();
        
        HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE) InitThread, hModule, 0, nullptr);
        if (hThread != nullptr)
        {
            CloseHandle(hThread);
        }  
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        IHHook::Shutdown();
        // DInputProxy
        if (origDll)
        {
            FreeLibrary(origDll);
        }
    }

    return TRUE;
} // DllMain
