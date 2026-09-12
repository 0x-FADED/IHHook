#include "IHHook.h"
#include "windowsapi.h"
#include <filesystem>

#include "Hooks_FOV.h" //DEBUGNOW
#include "ntdll.h"

HMODULE g_thisModule;
extern HMODULE origDll; // dinputproxy

static void initialize()
{
    auto anti_anti_dbg = []() -> void
    {
            DWORD64 dwAddr = (DWORD64)GetProcAddress(GetModuleHandleW(L"KERNELBASE.dll"), "IsDebuggerPresent");
            DWORD buf;

            bool result = VirtualProtect((BYTE*)dwAddr, 0x20, PAGE_EXECUTE_READWRITE, &buf);
            if (result == NULL)
            {
                spdlog::error("Failed to patch anti-dbg checks");
            }
            else
            {
                *(BYTE*)(dwAddr) = 0xB8;
                std::memset((BYTE*)(dwAddr + 0x01), 0x00, 0x04);
                *(BYTE*)(dwAddr + 0x5) = 0xC3;
                VirtualProtect((BYTE*)dwAddr, 0x20, buf, &buf);
                FlushInstructionCache(GetCurrentProcess(), (BYTE*)dwAddr, 0x20);
            }

            PPEB peb = (PPEB)__readgsqword(0x60);
            peb->BeingDebugged = false;
			peb->NtGlobalFlag &= ~0x70;

			//dx11 anti-anti-hook
            constexpr const uint8_t bytes[]{ 0xEB, 0x2D };
            hook::patch(hook::get_pattern<uint8_t>("75 2D FF 15 ? ? ? ? 49 8B 14 FF"), bytes);
        
    };

    anti_anti_dbg();
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