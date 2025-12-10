#include "Hooks_TPP.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "HookMacros.h"
#include "IHHook.h" //BaseAddr
extern "C"
{
#include "MinHook/MinHook.h"
}
#include "hooks/mgsvtpp_func_typedefs.h"
#include "spdlog/spdlog.h"

namespace IHHook
{
std::map<short, __int64> locationLangIds{
    {10, 0x1b094033d45d}, // afgh,tpp_loc_afghan
    {20, 0x7114b69e71e7}, // mafr,tpp_loc_africa
    {50, 0xfa8eaa7758b1}, // mtbs,tpp_loc_mb

    // DEBUGNOW proof of concept hack
    //{40,0x27376b6e62ff},//tpp_loc_gntn - caplags langid from his gntn addon
};

namespace Hooks_TPP
{
// tex from here
// https://discord.com/channels/364177293133873153/364178190588968970/698650439817625691
//(though still not sure how partoftheworlD recognised this in the first place)
// If you memory dump the exe after execution of this point ghidra recognises
// this as entry point in the dumped exe eyeballing the function it seems to be
// _mainCRTStartup
// https://stackoverflow.com/questions/22934206/what-is-the-difference-between-main-and-maincrtstartup
//"mainCRTStartup basically looks like this:
// init_tls();
// init_crt();
// run_global_constructors();
// get_args(&argc, &argv);
// ret = main(argc, argv);
// run_global_destructors();
// exit(ret);
//.So, main is in there, some place.– Damon Apr 8 '14 at 11:03"
// tex so you can find actual main from this
// not much point hooking it or actual main (lets call it FoxMain to be clearer)
// at the moment since IHHook is currently a dinput8 proxy which is obviously
// well past the _crtMain/FoxMain execute point

// uintptr_t missionCode_Addr = 0x142A58A00;
// uint32_t* missionCode;//tex in header

// TODO: move to exploration
// void UnkSomePlayerUpdateFuncHook(intptr_t unkPlayerClass, uintptr_t
// playerIndex) { 	spdlog::trace(__func__);
//	UnkSomePlayerUpdateFunc(unkPlayerClass, playerIndex);

//	intptr_t playerClass = unkPlayerClass;
//
//}//UnkSomePlayerUpdateFuncHook

////Address of signature = mgsvtpp_1_0_15_1_en.exe + 0x012C7570//15.1
//(UnkAnotherPlayerUpdateFuncButHuge)// 0x1412cf110 = 15.3 DEBUGNOW

// tex the idroid free roam mission tab had an issue where it wouldn't show the
// name of custom free roam missions despite there being a
// map_location_parameter - locationNameLangId = "tpp_loc_<whatever> (that
// matches tpp_common lng for vanilla free) however the above map does show
// given that there's a location icon I guess that's set up in engine
// See IH InfMission.EnableLocationChangeMissions
// searching for the hashes of the mentioned tpp_loc<> (kept for ref) in the exe
// finds this function returns strcode64 IN: locationLangIds
long long* GetFreeRoamLangIdHook(__int64* langId, const short locationCode, const short missionCode)
{
    spdlog::trace(__func__);

    if (locationCode == 50 && missionCode == 30150)
    {
        *langId = 0xe3d47a6e1e15; // tpp_loc_mb_zoo
    }
    else
    {
        auto it = locationLangIds.find(locationCode);
        if (it != locationLangIds.end())
        {
            *langId = it->second; // Found the location code
        }
        else
        {
            *langId = 0xb8a0bf169f98; // Empty string
        }
    }

    return langId;
} // GetFreeRoamLangIdHook

// DEBUGNOW not really tpp only Hooks_Fox?
static void UnkPrintFuncStubbedOutHook(const char* fmt, ...)
{
    spdlog::trace(__func__);
    va_list args;
    va_start(args, fmt);

    int size = 100;
    std::string message;
    va_list ap;

    while (1)
    {
        message.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf(&message[0], size, fmt, ap);
        va_end(ap);

        if (n > -1 && n < size)
        {
            message.resize(n); // Make sure there are no trailing zero char
            break;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    } // while(1)

    spdlog::debug(message);
} // UnkPrintFuncStubbedOutHook

void nullsub_2Hook(const char* unkSomeIdStr, unsigned long long unkSomeIdNum)
{
    // spdlog::trace(__func__);
    if (unkSomeIdStr != NULL)
    {
        try
        {
            char idStr[1024];
            fmt::format_to(idStr, "{}\0", unkSomeIdStr);
            spdlog::debug("nullsub_2 {}", unkSomeIdStr);
        }
        catch (...)
        {
        }
    }
} // nullsub_2Hook

void CreateHooks()
{
    spdlog::trace(__func__);

    if (addressSet["StrCode64"] == NULL)
    {
        spdlog::warn("addr fail: addressSet[\"StrCode64\"] == NULL");
    }
    else
    {
        // DEBUGNOW TEST
        const char* langId = "tpp_loc_gntn";
        long long tpp_loc_gntnS64 = StrCode64(langId, std::char_traits<char>::length(langId));

        spdlog::debug("Str64 tpp_loc_gntn: {:#x}", tpp_loc_gntnS64);

        // 0x1b094033d45d//tpp_loc_afghan
        //{ 20,0x7114b69e71e7 },//mafr,tpp_loc_africa
        //{ 50,0xfa8eaa7758b1 },//mtbs,tpp_loc_mb
        ////DEBUGNOW proof of concept hack
        //{ 40,0x27376b6e62ff },//tpp_loc_gntn - caplags langid from his gntn
        // addon
    }

    if (addressSet["GetFreeRoamLangId"] == NULL || addressSet["UnkPrintFuncStubbedOut"] == NULL || addressSet["nullsub_2"] == NULL)
    {
        spdlog::warn("addr == NULL");
    }
    else
    {
        CREATE_HOOK(GetFreeRoamLangId)
        CREATE_HOOK(UnkPrintFuncStubbedOut)
        //	CREATE_HOOK(nullsub_2)

        ENABLEHOOK(GetFreeRoamLangId)

        ENABLEHOOK(UnkPrintFuncStubbedOut) // DEBUGNOW
#ifdef _DEBUG
        // ENABLEHOOK(nullsub_2)//DEBUGNOW
#endif // DEBUG
    }  // if addr

    // DEBUGNOW
    // CREATE_HOOK(UnkSomeUpdateFunc)
    // ENABLEHOOK(UnkSomeUpdateFunc)
} // CreateHooks
} // namespace Hooks_TPP
} // namespace IHHook