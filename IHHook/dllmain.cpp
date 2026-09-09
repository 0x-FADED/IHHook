#include "IHHook.h"
#include "windowsapi.h"
#include <filesystem>

#include "Hooks_FOV.h" //DEBUGNOW

HMODULE g_thisModule;
extern HMODULE origDll; // dinputproxy

static void initialize()
{
    constexpr const uint8_t bytes[]{ 0xEB, 0x2D };
    hook::patch(hook::get_pattern<uint8_t>("75 2D FF 15 ? ? ? ? 49 8B 14 FF"), bytes);

    g_ihhook = std::make_unique<IHHook::IHH>();
    g_ihhook->Initialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        g_thisModule = hModule;

        initialize();

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