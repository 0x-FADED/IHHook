#include "IHHook.h"

#include <imgui/imgui.h>
#include <signal.h>
#include <winternl.h>

#include <ctime>
#include <filesystem>
#include <string>

#include "Hooks_Buddy.h" //ZIP: For buddies
#include "Hooks_Character.h"
#include "Hooks_CityHash.h"
#include "Hooks_FOV.h"
#include "Hooks_FnvHash.h"
#include "Hooks_FoxString.h" //ZIP: FoxString hook
#include "Hooks_LoadFile.h"
#include "Hooks_Lua.h"
#include "Hooks_TPP.h"
#include "Hooks_Vehicle.h"   //ZIP: For vehicles
extern "C"
{
#include "MinHook/MinHook.h"
} // MH_Initialize
#include "OS.h"
#include "PipeServer.h"
#include "RawInput.h"
#include "imguiimpl/imgui_impl_dx11.h"
#include "imguiimpl/imgui_impl_win32.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
// version_info parse
#include <fstream>
#include <sstream>

#include "IHMenu.h"
#include "StyleEditor.h"
#include "Util.h" //config
#include "hooks/mgsvtpp_adresses_1_0_15_0_en.h"
#include "hooks/mgsvtpp_adresses_1_0_15_3_jp.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // tex see note in imgui_impl_win32.h

constinit std::unique_ptr<IHHook::IHH> g_ihhook{};

namespace IHHook
{
// mgsvtpp_funcptr_set.cpp
extern void SetFuncPtrs();
extern void CreateHooks();

struct Config config;
bool ParseConfig(std::string fileName);

std::atomic<bool> doShutDown = false;

std::vector<std::string> errorMessages{};

bool isTargetExe = false;
ankerl::unordered_dense::map<std::string, uint64_t> addressSet{};

terminate_function terminate_Original;

void AbortHandler(int signal_number)
{
    auto log = spdlog::get("ihhook");
    if (log != NULL)
    {
        log->error("abort was called");
        log->flush();
    }
} // AbortHandler

void TerminateHandler()
{
    auto log = spdlog::get("ihhook");
    if (log != NULL)
    {
        log->error("terminate was called");
        log->flush();
    }
    terminate_Original();
} // TerminateHandler

bool g_showCrashDialog = true;
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* /*ExceptionInfo*/)
{
    auto log = spdlog::get("ihhook");
    if (log != NULL)
    {
        log->error("Unhandled exception");
        log->flush();
    }

    return g_showCrashDialog ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
} // UnhandledExceptionHandler

LONG WINAPI UnhandledExceptionFilter_Hook(EXCEPTION_POINTERS* /*ExceptionInfo*/)
{
    // When the CRT calls SetUnhandledExceptionFilter with NULL parameter
    // our handler will not get removed.
    auto log = spdlog::get("ihhook");
    if (log != NULL)
    {
        log->error("Unhandled exception H");
        log->flush();
    }

    return 0;
} // UnhandledExceptionFilter_Hook

typedef LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI* SetUnhandledExceptionFilter_Type)(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
SetUnhandledExceptionFilter_Type SetUnhandledExceptionFilter_Orig = NULL;

typedef BOOL(WINAPI* SetCursorPosFunc)(int, int);
SetCursorPosFunc SetCursorPos_Orig = NULL;

BOOL WINAPI SetCursorPos_Hook(int X, int Y)
{
    if (g_ihhook->IsUnlockCursor())
        return FALSE;

    return SetCursorPos_Orig(X, Y);
} // SetCursorPos_Hook

void InitCursorHook()
{
    auto log = spdlog::get("ihhook");

    if (MH_CreateHook(&SetCursorPos, &SetCursorPos_Hook, reinterpret_cast<LPVOID*>(&SetCursorPos_Orig)) != MH_OK)
    {
        log->error("Couldn't create hook for SetCursorPos.");
        return;
    }

    if (MH_EnableHook(&SetCursorPos) != MH_OK)
    {
        log->error("Couldn't enable SetCursorPos hook.");
    }
    
} // InitCursorHook

typedef HMODULE(WINAPI* LoadLibraryAFn)(LPCSTR);
LoadLibraryAFn g_LoadLibraryA = NULL;

bool IHH::isD3D11Loaded = false;
HMODULE WINAPI LoadLibraryA_Hook(LPCSTR lpFileName) 
{ 
    if (strcmp(lpFileName, "ole32.dll") == 0)
    {
        spdlog::info("LoadLibraryA_Hook: {:} is loaded", lpFileName);
        g_ihhook->isD3D11Loaded = true;
    }

    return g_LoadLibraryA(lpFileName); 
}

void InitLoadLibraryAHook()
{ 
   auto log = spdlog::get("ihhook");

   if (MH_CreateHook(&LoadLibraryA, &LoadLibraryA_Hook, reinterpret_cast<LPVOID*>(&g_LoadLibraryA)) != MH_OK)
   {
       log->error("Couldn't create hook for LoadLibraryA.");
   }

   if (MH_EnableHook(&LoadLibraryA) != MH_OK)
   {
       log->error("Couldn't enable LoadLibraryA hook.");
   }
}

void Shutdown()
{
    spdlog::debug("IHHook DLL_PROCESS_DETACH");
    RawInput::UninitializeInput();
    doShutDown = true;
    PipeServer::ShutDownPipeServer();
} // Shutdown

// GOTCHA: only set up stuff that can be done in this point of fox engine
// execution (when it's loading this dinput8.dll proxy) see Initialize for stuff
// after
IHH::IHH()
    : RealBaseAddr(*reinterpret_cast<SIZE_T*>(__readgsqword(0x60) + 0x10)), hwnd(0)
{
    signal(SIGABRT, &AbortHandler); // tex signal handler for SIGABRT which is
                                    // thrown by abort()
    terminate_Original = set_terminate(TerminateHandler);
    _set_abort_behavior(1, _WRITE_ABORT_MSG);

    SetUnhandledExceptionFilter(UnhandledExceptionHandler);

    // https://www.codeproject.com/Articles/154686/SetUnhandledExceptionFilter-and-the-C-C-Runtime-Li
    // if (MH_CreateHook(&SetUnhandledExceptionFilter,
    // &UnhandledExceptionFilter_Hook,
    // reinterpret_cast<LPVOID*>(&SetUnhandledExceptionFilter_Orig)) != MH_OK) {
    //	//DEBUGNOW message error
    //	return 1;
    // }
    // MH_EnableHook(SetUnhandledExceptionFilter);

    if (config.openConsole)
    {
        AllocConsole();
        SetConsoleTitle(L"IHHook");
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
        printf("Console test\n");
    }

    // tex DEBUG, logged below
    TCHAR Buffer[MAX_PATH];
    DWORD dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
    std::wstring currentDir(Buffer);

    std::wstring gameDir = OS::GetGameDir();
    SetCurrentDirectory(gameDir.c_str()); // tex so this dll and lua can use reletive paths

    config.debugMode = true; // DEBUGNOW -v
    SetupLog();
    ParseConfig(hookConfigName); // TODO: set log level via config.debugMode

    // tex DEBUGNOW mgo is a seperate exe in the same dir, so bail out on exe
    // name
    HMODULE hExe = GetModuleHandle(NULL);
    WCHAR fullPath[MAX_PATH]{0};
    GetModuleFileNameW(hExe, fullPath, MAX_PATH);
    std::filesystem::path path(fullPath);
    std::wstring exeName = path.filename().c_str();
    if (exeName.find(L"mgo") != std::wstring::npos)
    {
        log->warn("IHHook is not for mgo");
        return;
    }
    //

    log->debug(L"Original CurrentDir: {}", currentDir.c_str());
    log->debug(L"gameDir: {}", gameDir);

#ifdef _DEBUG
    std::vector<std::string> modFileNames = OS::GetFileNames("./mod");
    std::vector<std::string> folderNames = OS::GetFolderNames("./mod");
#endif // _DEBUG

    if (!std::filesystem::exists("./mod/modules"))
    { // tex GOTCHA: since this continues ih_log will
      // be created thus ./mod will actually exist. so
      // check modules instead
        errorMessages.push_back("ERROR: IH mod folder not found.");

        for (auto& message : errorMessages)
        {
            log->error(message);
        }
    }

    // tex Much of IHHooks hooks are based on direct addresses, so if the exe is
    // different the user needs to know can just hope that konami actually keeps
    // updating the exe version properly and not release multiple updates with
    // no exe version change like they have in the past but version_info.txt
    // should help there too

    std::string lang = GetLangVersion();

    std::string exeVersionStr = "";
    int versionDelta = OS::CheckVersionDelta(IHHook::GameVersion, exeVersionStr);
    if (versionDelta != 0)
    {
        isTargetExe = false;

        errorMessages.push_back("ERROR: IHHook->exe version mismatch");
        errorMessages.push_back("Infinite Heaven will continue to load");
        errorMessages.push_back("with some limitations.");
        errorMessages.push_back("Including this menu not working in-game.");
        if (versionDelta > 0)
        {
            errorMessages.push_back("Please update MGSV.");
        }
        else if (versionDelta < 0)
        {
            errorMessages.push_back("Please update Infinte Heaven.");
        }
        errorMessages.push_back("Click on the x to close this window.");

        for (auto& message : errorMessages)
        {
            log->error(message);
        }
        SetCursor(true); // tex DEBUGNOW imgui window currently wont auto
                         // dismiss, so give user cursor
    }
    else
    {
        if (lang != "en" && lang != "jp")
        { // DEBUGNOW
            isTargetExe = false;

            errorMessages.push_back("WARNING: Unknown lang version");
            errorMessages.push_back("Infinite Heaven will continue to load");
            errorMessages.push_back("with some limitations.");
            errorMessages.push_back("Including this menu not working in-game.");
            errorMessages.push_back("Click on the x to close this window.");

            for (auto& message : errorMessages)
            {
                log->error(message);
            }
            SetCursor(true); // tex DEBUGNOW imgui window currently wont auto
                             // dismiss, so give user cursor
        }
        else
        {
            // tex for using listed address vs sigscan (but not actually
            // currently doing so, see doHooks comment)
            isTargetExe = true;
        } //
    }     // ChecKVersion

    bool doHooks = isTargetExe; // tex not actually doing hooks if not target exe. in
                                // theory could fall back to signature scanning, however it
                                // takes a litteral minute for 100+ signatures to be found
    // plus if you did go that route you'd have to put it at an earlier blocking
    // point (like off dllmain itself) since this function we're in is run by a
    // thread so the exe will continue past the point we need our hooks up and
    // running But heres a config option to test

    if (doHooks)
    { // tex hook em up boys
        Hooks_Lua::SetupLog();

        MH_Initialize();

        // GAMEVERSION
        // DEBUGNOW TODO: an adresset map too I guess
        if (lang == "en")
        {
            addressSet = mgsvtpp_adresses_1_0_15_0_en;
        }
        else
        {
            if (lang == "jp")
            {
                addressSet = mgsvtpp_adresses_1_0_15_3_jp;
            }
            else
            {
                // tex unknown exe lang, should already be handled by
                // isTargetExe
            }
        } // if lang

        auto tstart = std::chrono::high_resolution_clock::now();

        bool foundAllAddresses = RebaseAddresses(isTargetExe);

        if (!foundAllAddresses)
        {
            log->warn("Could not find all addresses");
        }
        else
        {
            SetFuncPtrs();
            // DEBUGNOW CreateHooks();
        }

        CreateAllHooks();

        auto tend = std::chrono::high_resolution_clock::now();
        auto durationShort = std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
        log->debug("IHHook::CreateHooks total time(microseconds): {}µs", durationShort);
    } // if doHooks

    PipeServer::StartPipeServer();

    log->debug("IHH ctor complete");
    log->flush();
} // IHH

IHH::~IHH()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    log->info("mod uninitialized");
    spdlog::shutdown();
} //~IHH

// CALLER: thread spawned by dllmain
// GOTCHA: KLUDGE: see comment in dllmain
void IHH::Initialize()
{
    CreateD3DHook();
    RemoveDebuggerCheck();
}

// OUT/SIDE: log file, log file prev
// OUT/SIDE: log
void IHH::SetupLog()
{
    DeleteFile(IHHook::hookLogNamePrev.c_str());
    CopyFile(IHHook::hookLogName.c_str(), IHHook::hookLogNamePrev.c_str(), false);
    DeleteFile(IHHook::hookLogName.c_str());

    log = spdlog::basic_logger_mt("ihhook", IHHook::hookLogName); // DEBUGNOW st vs mt
    spdlog::set_default_logger(log);

    log->set_pattern("|%H:%M:%S:%e|%l: %v");
    log->info("IHHook r{}", IHHook::Version);

    if (config.debugMode)
    {
        log->set_level(spdlog::level::trace);
        log->flush_on(spdlog::level::trace);
    }
    else
    {
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::err);
    }
    
    SYSTEMTIME st = {};
    GetLocalTime(&st);

    log->info("Started: {:04}/{:02}/{:02} {:02}:{:02}:{:02}", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    log->debug("Note: ihhook_log is multithreaded to accept logging from multiple threads so order of entries may not be sequential.");
    log->flush();
} // SetupLog

void IHH::CreateD3DHook()
{
    d3d11Hook = std::make_unique<D3D11Hook>();
    d3d11Hook->on_present([this](D3D11Hook& hook) { OnFrame(); });
    d3d11Hook->on_resize_buffers([this](D3D11Hook& hook) { OnReset(); });

    d3dHooked = d3d11Hook->hook();
    if (d3dHooked)
    {
        log->info("Hooked D3D11");
    }
    else
    {
        if (std::filesystem::exists("d3d11.dll"))
        {
            std::wstring title = L"MGSTPP - Infinite Heaven IHHook";
            std::wstring message = L"ERROR: Could not hook D3D11\n"
                                   L"Unknown d3d11.dll in MGS_TPP folder\n"
                // DEBUGNOW L"If this is from the FOV Modifier dll you can
                // remove it\n" L"as IHHook now has it intergrated\n"
                ;
            MessageBox(NULL, message.c_str(), title.c_str(), NULL);
        }
        else
        {
            std::wstring title = L"MGSTPP - Infinite Heaven IHHook";
            std::wstring message = L"ERROR: Could not hook D3D11\n"
                                   L"See ihhook_log.txt in MGS_TPP folder for details.\n";
            MessageBox(NULL, message.c_str(), title.c_str(), NULL);
        } // exists d3d11.dll

    } // d3dHooked
} // CreateD3DHook

std::string IHH::GetLangVersion()
{
    // DEBUGNOW So jp voice version is actually different exe, so cant just rely
    // on exe version info.
    std::string versionInfoFileName = "version_info.txt";
    std::ifstream infile(versionInfoFileName);
    if (infile.fail())
    { // tex likely pirated game, or user has some wierd
      // setup, cant know actual version
        log->warn("Could not load ", versionInfoFileName);
        log->warn("Cannot differentiate what language version the exe is, "
                     "so game may "
                     "crash when hooking if exe version matches but using "
                     "different sku.");
        // any point using errormessages since if this is an actual lang exe
        // mismatch its going to crash before it gets to the ui DEBUGNOW think
        // what to do.
    }

    // REF
    // Tpp_steam_mst_en_day1820Mgo_patch_0212_1307
    // Tpp_steam_mst_jp_day1820Mgo_patch_0212_1307
    std::string line;
    std::string lang = "";
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);

        if (line.length() < std::string("Tpp_steam_mst_en").length())
        {
            log->warn("Unexpected version string, string shorter than expected");
            break;
        }

        std::string prefix = "Tpp_steam_mst_";
        std::size_t found = line.find(prefix);
        if (found == std::string::npos)
        {
            log->warn("Unexpected version string, could not find {}", prefix);
            break;
        }

        lang = line.substr(prefix.length(), 2); // en,jp etc
        log->debug("Found lang: {}", lang);

        if (lang != "en" && lang != "jp")
        {
            log->warn("Unexpected lang version");
        }
        else
        {
            break;
        }
    } // while infile
    return lang;
} // GetLangVersion

// D3D11Hook->present
// GOTCHA: this is blocking to actual d3d Present, so keep performance in mind
void IHH::OnFrame()
{
    // GOTCHA: frameInitialized is reset in OnReset, so if you want something to
    // run only once a session use firstFrame in FramInisialize instead
    if (!frameInitialized)
    {
        if (!FrameInitialize())
        {
            log->error("Failed to frame initialize IHHook");
            return;
        }

        log->info("IHHook frame initialized");
        frameInitialized = true;
        return; // tex give it an extra frame to settle I guess?
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawUI();

    ImGui::EndFrame();
    ImGui::Render();

    ID3D11DeviceContext* context = nullptr;
    d3d11Hook->get_device()->GetImmediateContext(&context);

    context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
} // OnFrame

// D3D11Hook
void IHH::OnReset()
{
    // DEBUGNOW
    auto log = spdlog::get("ihhook");
    log->info("OnReset");

    if (log != NULL)
    {
        log->flush();
    }

    // RE2FW: Crashes if we don't release it at this point.
    CleanupRenderTarget();
    frameInitialized = false;

    // DEBUGNOW
    log->info("OnReset done");
    if (log != NULL)
    {
        log->flush();
    }

} // OnReset

// WindowsMessageHook
bool IHH::OnMessage(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    // log->trace("OnMessage");

    if (!frameInitialized)
    {
        return true;
    }

    bool handledMessage = !RawInput::OnMessage(wnd, message, w_param, l_param);

    if (drawUI && ImGui_ImplWin32_WndProcHandler(wnd, message, w_param, l_param) != 0)
    {
        // RE2FW: If the user is interacting with the UI we block the message
        // from going to the game.
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput)
        {
            handledMessage = true;
        }
    }

    if (handledMessage)
    {
        // tex DEBUGNOW WORKAROUND: having menu eat all game can cause a problem
        // if user was holding a key at the time as the keyup even will be eaten
        if (w_param == WM_KEYUP)
        {
            return true;
        }

        return false; // tex eat the message
    }
    return true;
} // OnMessage

// tex called on initialize and on device reset
bool IHH::FrameInitialize()
{
    if (frameInitialized)
    {
        return true;
    }

    log->info("Attempting to frame initialize");

    auto device = d3d11Hook->get_device();
    auto swapChain = d3d11Hook->get_swap_chain();

    // Wait.
    if (device == nullptr || swapChain == nullptr)
    {
        log->info("Device or SwapChain null. DirectX 12 may be in use. A crash "
                     "may occur.");
        return false;
    }

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);

    DXGI_SWAP_CHAIN_DESC swapDesc{};
    swapChain->GetDesc(&swapDesc);

    hwnd = swapDesc.OutputWindow;

    // RE2FW: Explicitly call destructor first
    windowsMessageHook.reset();
    windowsMessageHook = std::make_unique<WindowsMessageHook>(hwnd);
    windowsMessageHook->on_message = [this](auto wnd, auto msg, auto wParam, auto lParam) { return OnMessage(wnd, msg, wParam, lParam); };

    log->info("Creating render target");

    CreateRenderTarget();

    log->info("Window Handle: {:#x}", (uintptr_t) hwnd);
    log->info("Initializing ImGui");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
    // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
    // Enable Gamepad Controls

    log->info("Initializing ImGui Win32");

    if (!ImGui_ImplWin32_Init(hwnd))
    {
        log->error("Failed to initialize ImGui.");
        return false;
    }

    log->info("Initializing ImGui D3D11");

    if (!ImGui_ImplDX11_Init(device, context))
    {
        log->error("Failed to initialize ImGui.");
        return false;
    }

    ImGui::StyleColorsDark();

    if (firstFrame)
    {
        firstFrame = false;

        RawInput::InitializeInput();

        IHMenu::AddMenuCommands();

        InitCursorHook();

        InitStyleEditor(); // StyleEditor

        IHMenu::SetInitialText();
    } // if firstFrame

    log->info("frame initialized");
    return true;
} // FrameInitialize

void IHH::CreateRenderTarget()
{
    CleanupRenderTarget();

    ID3D11Texture2D* backBuffer{nullptr};
    if (d3d11Hook->get_swap_chain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) &backBuffer) == S_OK)
    {
        d3d11Hook->get_device()->CreateRenderTargetView(backBuffer, NULL, &mainRenderTargetView);
        backBuffer->Release();
    }
} // CreateRenderTarget

void IHH::CleanupRenderTarget()
{
    auto log = spdlog::get("ihhook");
    log->trace("CleanupRenderTarget");

    if (log != NULL)
    {
        log->flush();
    }

    if (mainRenderTargetView != nullptr)
    {
        mainRenderTargetView->Release();
        mainRenderTargetView = nullptr;
    }
} // CleanupRenderTarget

void IHH::DrawUI()
{
    // std::lock_guard _{ inputMutex };//DEBUGNOW

    IHMenu::ProcessMessages();

    auto& io = ImGui::GetIO();
    if (!drawUI)
    {
        RawInput::UnBlockMouseClick();
        RawInput::UnBlockKeyboard();
        unlockCursor = false;
        io.MouseDrawCursor = false;
        return;
    }

    // tex disable mouse input to game
    if (unlockCursor)
    {
        ImGui::SetNextFrameWantCaptureMouse(true);
    }

    if (io.WantCaptureMouse)
    {
        RawInput::BlockMouseClick();
    }
    else
    {
        RawInput::UnBlockMouseClick();
    }

    if (io.WantCaptureKeyboard)
    {
        RawInput::BlockKeyboard();
    }
    else
    {
        RawInput::UnBlockKeyboard();
    }

    io.MouseDrawCursor = unlockCursor;

    if (showStyleEditor)
    {
        ShowStyleEditor(&showStyleEditor, showStyleEditorPrev, NULL);
        showStyleEditorPrev = showStyleEditor;
    }

    if (showImguiDemo)
    {
        ImGui::ShowDemoWindow(&showImguiDemo);
    }

    if (menuOpen)
    {
        IHMenu::DrawMenu(&menuOpen, menuOpenPrev);
    }
    if (!menuOpen && menuOpenPrev)
    {
        IHMenu::QueueMessageIn("menuoff");
    }
    menuOpenPrev = menuOpen;

    // ImGui::End();
} // DrawUI

// TODO: move to own file
// tex: even though it's saved as valid lua, we'll just parse it as text on
// IHHook side rather than dealing with back and forth through lua, and so
// IHHook can use it before lua is stood up
bool ParseConfig(std::string fileName)
{
    spdlog::debug("ParseConfig {}", fileName);
    std::ifstream infile(fileName);
    if (infile.fail())
    {
        spdlog::warn("ParseConfig ifstream.fail for {}", fileName);
        return false;
    }

    config.debugMode = true; // TODO debug level instead
    config.openConsole = false;
    config.enableCityHook = false;
    config.enableFnvHook = false;
    config.logFileLoad = false;
    config.logFoxStringCreateInPlace = false; // ZIP: Fox hooks

    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        // tex trim leading/trailing whitespace
        line = trim(line);
        if (line.size() == 0)
        {
            continue;
        }

        // tex deal with comments
        std::size_t found = line.find("--");
        // tex line is only comment
        if (found == 0)
        {
            continue;
        }
        // tex line has comment, trim to before comment
        if (found != std::string::npos)
        {
            line = line.substr(0, found - 1);
        }

        if (line.size() == 0)
        {
            continue;
        }

        // tex just skip the specific cases outright
        found = line.find("local this");
        if (found != std::string::npos)
        {
            continue;
        }
        found = line.find("return this");
        if (found != std::string::npos)
        {
            continue;
        }
        if (line == "}")
        {
            continue;
        }

        // tex trim trailing comma
        if (line[line.size() - 1] == ',')
        {
            line = line.substr(0, line.size() - 1);
        }

        found = line.find("=");
        if (found == std::string::npos)
        {
            continue;
        }

        std::string varName = line.substr(0, found);
        std::string valueStr = line.substr(found + 1);
        varName = trim(varName);
        valueStr = trim(valueStr);

        // tex ugh
        if (varName == "debugMode")
        {
            config.debugMode = valueStr == "true";
        }
        else if (varName == "openConsole")
        {
            config.openConsole = valueStr == "true";
        }
        else if (varName == "enableCityHook")
        {
            config.enableCityHook = valueStr == "true";
        }
        else if (varName == "enableFnvHook")
        {
            config.enableFnvHook = valueStr == "true";
        }
        else if (varName == "logFileLoad")
        {
            config.logFileLoad = valueStr == "true";
        }
        else if (varName == "logFoxStringCreateInPlace")
        { // ZIP: Fox hooks
            config.logFoxStringCreateInPlace = valueStr == "true";
        }
    } // while line

    return true;
} //

void IHH::RemoveDebuggerCheck()
{
    DWORD64 dwAddr = (DWORD64) GetProcAddress(GetModuleHandleW(L"KernelBase.dll"), "IsDebuggerPresent");
    DWORD dwVirtualProtectBackup;
    bool result = VirtualProtect((BYTE*) dwAddr, 0x20, PAGE_READWRITE, &dwVirtualProtectBackup);
    if (result == NULL)
        log->error("Failed to patch debugging.");
    else
    {
        *(BYTE*) (dwAddr) = 0xB8;
        memset((BYTE*) (dwAddr + 0x1), 0x0, 0x4);
        *(BYTE*) (dwAddr + 0x5) = 0xC3;
        VirtualProtect((BYTE*) dwAddr, 0x20, dwVirtualProtectBackup, &dwVirtualProtectBackup);
        log->debug("IsDebuggerPresent patched");
    }
    PPEB peb = (PPEB) __readgsqword(0x60);
    peb->BeingDebugged = false;
    *(DWORD*) ((char*) peb + 0xBC) &= ~0x70;
    log->info("Patched debugger checks.");
}

// IN: BaseAddr, RealBaseAddr
// IN: mgsvtpp_patterns
// SIDE: addressSet
// rebases the static addresses or sig scans for them
bool IHH::RebaseAddresses(bool isTargetExe)
{
    if (!isTargetExe)
    {
        log->error("Mismatch executable!");
        return false;
    }

    bool foundAllAddresses = true;
    for (auto& [name, addr] : addressSet)
    {
        // Rebase the address
        uint64_t rebasedAddr = (addr - BaseAddr) + RealBaseAddr;
        addr = rebasedAddr;

        // Log the rebasing
        log->info("Rebased address for {}", name);
    }

    return foundAllAddresses;
}

void IHH::CreateAllHooks()
{
    InitLoadLibraryAHook();
    Hooks_CityHash::CreateHooks(RealBaseAddr); // TODO: rebase/convert to same style as rest, so don't
                                              // have to pass in realbaseaddr
    Hooks_FNVHash::CreateHooks();
    Hooks_Lua::CreateHooks();
    Hooks_TPP::CreateHooks();
    Hooks_FOV::CreateHooks();
    Hooks_LoadFile::CreateHooks(); // DEBUGNOW exploring
    Hooks_Character::CreateHooks();
    Hooks_Buddy::CreateHooks();     // ZIP: For buddies
    Hooks_Vehicle::CreateHooks();   // ZIP: For vehicles
    Hooks_FoxString::CreateHooks(); // ZIP: FoxString hook
} // CreateAllHooks
} // namespace IHHook