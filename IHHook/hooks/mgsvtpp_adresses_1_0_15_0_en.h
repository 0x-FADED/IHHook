#pragma once
// GENERATED: by ghidra script ExportHooksToHeader.py
// via WriteAddressHFile

// NOT_FOUND - default for a lapi we want to use, and should actually have found
// the address in prior exes, but aren't in the current exported address list
// NO_USE - something we dont really want to use for whatever reason
// USING_CODE - using the default lapi code implementation instead of hooking

namespace IHHook
{
ankerl::unordered_dense::map<std::string, uint64_t> mgsvtpp_adresses_1_0_15_0_en{
    {"StrCode64", 0x14CB9B730}, //name according to fn: GetStrCodeWithLength;
    {"GetStrCode32", 0x142EAE000},
    {"PathCode64", 0x14CB9B4E0}, //tex TODO need to verify naming and purpose. technically this is PathFileNameExt64, but given that PathCode - without ext is likely less used than PathCode would have been a better name for PathFileNameExt64 or PathCode64Ext
    {"FNVHash32", 0x144254780},
    {"GetFreeRoamLangId", 0x14617CA30},
    {"UpdateFOVLerp", 0x14110ECB0},          // tex: TODO: verify the return AL>RAX
    {"UnkPrintFuncStubbedOut", 0x142FBB650}, // tex: Some info printing function that has been stubbed out
    {"l_StubbedOut", 0x140170CA0},           // tex: another retail stubb out to wrangle
    {"nullsub_2", 0x140DEB8C0},              // tex: another retail stubb out to wrangle
    {"LoadFileSub", 0x1430406E0},
    {"LoadFile", 0x1431D8D20},
    {"LoadFile_01", 0x1431D8510}, //name according to fn: Path_Copy
    {"LoadFile_02", 0x1431D93A0}, //name according to fn: ???
    {"LoadFile_03", 0x1431DC440}, //name according to fn: GetEmptyPath
    {"LoadFile_05", 0x1431D9670}, //name according to fn: ???
    {"LoadPlayerPartsFpk", 0x146C45EC0},
    {"LoadPlayerPartsParts", 0x146C45890},
    {"LoadPlayerCamoFpk", 0x146C42D20},
    {"LoadPlayerCamoFv2", 0x146C42500},
    {"LoadPlayerFacialMotionFpk", 0x146C45410},
    {"LoadPlayerFacialMotionMtar", 0x146C45170},
    {"LoadPlayerBionicArmFpk", 0x140AE2FC0},
    {"LoadPlayerBionicArmFv2", 0x140AE2F10},
    {"CheckPlayerPartsIfShouldApplySkinToneFv2", 0x140AE32D0},
    {"LoadPlayerPartsSkinToneFv2", 0x140AE2430},
    {"IsHeadNeededForPartsType", 0x140AE2380},
    {"IsHeadNeededForPartsTypeAndAvatar", 0x140AE23D0},
    {"LoadPlayerSnakeFaceFpk", 0x140AE2CC0},
    {"LoadPlayerSnakeFaceFv2", 0x140AE2BB0},
    {"LoadAvatarOgreHornFpk", 0x146C3A270},
    {"LoadAvatarOgreHornFv2", 0x146C39F60},
    {"LoadBuddyMainFile", 0x140A400C0},
    {"LoadBuddyQuietWeaponFpk", 0x1468DA220},
    {"LoadBuddyDogCommonFPK", 0x140A4059A},
    {"LoadBuddyHorseCommonFPK", 0x140A405A3},
    {"LoadBuddyWalkerGearArmFpk", 0x1468D8F70},
    {"LoadBuddyWalkerGearHeadFpk", 0x1468D93F0},
    {"LoadBuddyWalkerGearWeaponFpk", 0x1468D9710},
    {"LoadDefaultFpksFunc", 0x143183BB0},
    {"PreparePlayerVehicleInSortie", 0x146F3E290},
    {"PreparePlayerVehicleInGame", 0x146F3DF70},
    {"LoadDefaultFpkPtrFunc", 0x14317F300},
    {"LoadAllVehicleCamoFpks", 0x145021900},
    {"BuddyCommandGetNameLangId", 0x141106110},
    {"BuddyCommandGetDescriptionLangId", 0x141105EC0},
    {"CreateInPlace", 0x142E5C040},
    {"lua_newstate", 0x14CBD9730}, // tex could use default implementation, but may want to hook if we want to see what the engine is up to
    {"lua_close", 0x14CBD9210},
    {"lua_newthread", 0x14CBB71C0},
    {"lua_atpanic", 0x14CBB23F0},
    //{"lua_gettop", USING_CODE},
    {"lua_settop", 0x14CBBD720},
    {"lua_pushvalue", 0x14CBBA860},
    {"lua_remove", 0x14CBBB9C0},
    {"lua_insert", 0x141A04D50},
    {"lua_replace", 0x14CBBC030},
    {"lua_checkstack", 0x14CBB2680},
    {"lua_xmove", 0x14CBBF570},
    {"lua_isnumber", 0x14CBB6570},
    {"lua_isstring", 0x14CBB6730},
    {"lua_iscfunction", 0x14CBB5D20},
    //{"lua_isuserdata", USING_CODE},//tex: No calls in lua distro, so may be hard to find, or have been culled by compilation
    {"lua_type", 0x14CBBF030},
    //{"lua_typename", USING_CODE},
    // {"lua_equal", NOT_FOUND},//tex: lua implementation goes a bit deeper than I'm happy with to use at the moment. No calls in lua distro, so may be hard to find, or have been culled by compilation
    {"lua_rawequal", 0x14CBBAD60},
    {"lua_lessthan", 0x14CBB6DE0},
    {"lua_tonumber", 0x14CBBE2C0},
    {"lua_tointeger", 0x14CBBDFF0},
    {"lua_toboolean", 0x14CBBD810},
    {"lua_tolstring", 0x14CBBE110},
    {"lua_objlen", 0x14CBB7F10},
    {"lua_tocfunction", 0x14CBBDC50},
    {"lua_touserdata", 0x14CBBEC70},
    {"lua_tothread", 0x14CBBE800},
    {"lua_topointer", 0x14CBBE4D0},
    {"lua_pushnil", 0x14CBB9F50},
    {"lua_pushnumber", 0x14CBBA1E0},
    {"lua_pushinteger", 0x14CBB9370},
    {"lua_pushlstring", 0x14CBB9940},
    {"lua_pushstring", 0x14CBBA300},
    {"lua_pushvfstring", 0x14CBBAAB0},
    {"lua_pushfstring", 0x14CBB90B0},
    {"lua_pushcclosure", 0x14CBB8D10},
    {"lua_pushboolean", 0x14CBB89D0},
    {"lua_pushlightuserdata", 0x14CBB9690},
    {"lua_pushthread", 0x14CBBA7C0},
    {"lua_gettable", 0x14CBB5560},
    {"lua_getfield", 0x14CBB4D70},
    {"lua_rawget", 0x14CBBAEC0},
    {"lua_rawgeti", 0x14CBBB150}, // via MACRO lua_getref
    {"lua_createtable", 0x14CBB3420},
    {"lua_newuserdata", 0x14CBB7650},
    {"lua_getmetatable", 0x14CBB4F40},
    {"lua_getfenv", 0x14CBB4B40},
    {"lua_settable", 0x14CBBD450},
    {"lua_setfield", 0x14CBBC6E0},
    {"lua_rawset", 0x14CBBB300},
    {"lua_rawseti", 0x14CBBB690},
    {"lua_setmetatable", 0x14CBBD030},
    {"lua_setfenv", 0x14CBBC510},
    {"lua_call", 0x14CBB2490},
    {"lua_pcall", 0x14CBB8560},
    {"lua_cpcall", 0x147200FC0},
    {"lua_load", 0x14CBB6F40},
    {"lua_dump", 0x14CBB3620},
    //{"lua_yield", USING_CODE},//tex: DEBUGNOW uses lua_lock, may not be a good idea due to thread issues and not knowing what the engine is doing to the state. Seems to be inlined in luaB_yield (it's only call in lua distro)
    {"lua_resume", 0x14CBCB3A0},
    //{"lua_status", USING_CODE},//tex DEBUGNOW hmm, address range. ida  finds this as sig though, but the prior functions have entries in .pdata which put them in the same range (0x14cdb)
    {"lua_gc", 0x141A04970},
    {"lua_error", 0x14CBB4790},
    {"lua_next", 0x14CBB7C40},
    {"lua_concat", 0x14CBB2A20},
    //{"lua_getallocf", NO_USE},//tex don't really want to mess with allocator function anyway, DEBUGNOW no calls in lua distro, so may be hard to find, or have been culled by compilation
    //{"lua_setallocf", NO_USE},//tex don't really want to mess with allocator function anyway
    //{"lua_setlevel", NO_USE},//tex: labeled by lua as a hack to be removed in lua 5.2
    {"lua_getstack", 0x14CBEE930},
    {"lua_getinfo", 0x141A14500},
    {"lua_getlocal", 0x4CBEDBA0},
    {"lua_setlocal", 0x14CBEEE50},
    {"lua_getupvalue", 0x14CBB5A60},
    {"lua_setupvalue", 0x141A05990},
    {"lua_sethook", 0x141A05990},
    //{"lua_gethook", USING_CODE},
    //{"lua_gethookmask", USING_CODE},
    //{"lua_gethookcount", USING_CODE},
    {"luaI_openlib", 0x14CBE0910},
    //{"luaL_register", USING_CODE},
    {"luaL_getmetafield", 0x14CBDEF80},
    {"luaL_callmeta", 0x14CBDB6A0},
    {"luaL_typerror", 0x141A0BB60},
    {"luaL_argerror", 0x4CBDB010},
    {"luaL_checklstring", 0x14CBDC390},
    {"luaL_optlstring", 0x14CBE1040},
    {"luaL_checknumber", 0x14CBDC940},
    //{"luaL_optnumber", USING_CODE},//tex: Only use in os_difftime, but decompilation is giving a bunch more params than it usually takes
    {"luaL_checkinteger", 0x14CBDBB30},
    {"luaL_optinteger", 0x14CBE0D70},
    {"luaL_checkstack", 0x14CBDCDF0},
    {"luaL_checktype", 0x14CBDD760},
    {"luaL_checkany", 0x14CBDB8F0},
    {"luaL_newmetatable", 0x14CBE00B0},
    {"luaL_checkudata", 0x14CBDDF80},
    {"luaL_where", 0x14CBE1DD0},
    {"luaL_error", 0x14CBDEB10},
    {"luaL_checkoption", 0x14CBDCC00},
    //{"luaL_ref", USING_CODE},//tex: Unsure on this address. No uses in lua dist, found a function that looks much like it, but it was undefined, and has a errant param
    //{"luaL_unref", USING_CODE},
    {"luaL_loadfile", 0x141A0B230},
    {"luaL_loadbuffer", 0x14CBDF8B0},
    //{"luaL_loadstring", USING_CODE},
    {"luaL_newstate", 0x14CBE0560},
    {"luaL_gsub", 0x141A0ADB0},
    {"luaL_findtable", 0x14CBDED20},
    //{"luaL_buffinit", USING_CODE},
    {"luaL_prepbuffer", 0x14CBE14A0},
    {"luaL_addlstring", 0x141A0A510},
    //{"luaL_addstring", USING_CODE},//tex: Only call is in luaL_gsub, seems to have been optimized out as the function just wraps luaL_addlstring
    {"luaL_addvalue", 0x14CBDAE90},
    {"luaL_pushresult", 0x14CBE17A0},
    {"luaopen_base", 0x14CBFBC80},
    {"luaopen_table", 0x14CBFC280},
    {"luaopen_io", 0x14CBFC580},
    {"luaopen_os", 0x14CBFCA80},
    {"luaopen_string", 0x14CBFCC20},
    {"luaopen_math", 0x14CBFD070},
    {"luaopen_debug", 0x14CBFD480},
    {"luaopen_package", 0x14CBFD640},
    {"luaL_openlibs", 0x14CBD9FD0},
}; // map mgsvtpp_adresses_1_0_15_0_en
} // namespace IHHook
