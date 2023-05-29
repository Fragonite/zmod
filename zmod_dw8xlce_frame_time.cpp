// #define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <filesystem>
#include <map>
#include <vector>
#include <sstream>
#include "detours.h"
#include "zmod_common.cpp"

typedef void(__stdcall function_1C90D0_t)(LARGE_INTEGER *lpPerformanceCount, uint32_t vsync_30);
function_1C90D0_t game_wait_hook;

typedef void(__stdcall function_554050_t)();
function_554050_t timer_update_hook;
function_554050_t *_timer_update;

typedef int(__stdcall function_583320_t)();
function_583320_t hud_state_update_hook;
function_583320_t *_hud_state_update;

std::vector<uintptr_t> fild_vsync_en = {
    0x0022D155,
    0x00233E9B,
    0x0023B30F,
    0x00255316,
    0x002553D7,
    0x00255416,
    0x0030BB53,
    0x0035B9AB,
    0x0039E922,
    0x003C54C7,
    0x003C79F9,
    0x003CDD15,
    0x003CE190,
    0x003CF58B,
    0x003CFE03,
    0x003CFE45,
    0x003DAD58,
    0x003E063A,
    0x003EB43C,
    0x003EBB99,
    0x003EC488,
    0x003F3DD3,
    0x0042BA50,
    0x0043E216,
    0x0043E236,
    0x00441355,
    0x00441613,
    0x00441697,
    0x00445C65,
    0x00445CD5,
    0x00445DB9,
    0x00445DF5,
    0x0044C696,
    0x0044FB76,
    0x00452C80,
    0x004555DC,
    0x004622C6,
    0x004624BB,
    0x0046F052,
    0x00470715,
    0x00471926,
    0x00472470,
    0x00473283,
    0x004771AE,
    0x00478119,
    0x004863EA,
    0x00489567,
    0x00490EFC,
    0x0049100B,
    0x004B375C,
    0x004B61A8,
    0x004B6234,
    0x004B644B,
    0x004B65AE,
    0x004B7EC3,
    0x004C56F6,
    0x004CE22C,
    0x004CE274,
    0x004CE4AA,
    0x004CE4F9,
    0x004D1063,
    0x004D1175,
    0x004D1FD5,
    0x004D2230,
    0x004D22E5,
};

std::vector<uintptr_t> fild_vsync_zh = {
    0x0022D775,
    0x002344BB,
    0x0023B92F,
    0x00255946,
    0x00255A07,
    0x00255A46,
    0x002F3EB3,
    0x0033D03F,
    0x0039BCE2,
    0x003C3707,
    0x003C6779,
    0x003CA4F5,
    0x003CAA50,
    0x003CC02B,
    0x003CD1F3,
    0x003CD235,
    0x003D8138,
    0x003DDA1A,
    0x003E856C,
    0x003E8CC9,
    0x003E95B8,
    0x003F0EE3,
    0x00428B60,
    0x0043C046,
    0x0043C066,
    0x0043F185,
    0x0043F443,
    0x0043F4C7,
    0x00443A95,
    0x00443B05,
    0x00443BE9,
    0x00443C25,
    0x0044A4C6,
    0x0044D9A6,
    0x00450AB0,
    0x0045340C,
    0x004600F6,
    0x004602EB,
    0x0046CE82,
    0x0046E545,
    0x0046F756,
    0x004702A0,
    0x004710B3,
    0x00474FDE,
    0x00475F49,
    0x0048421A,
    0x00487397,
    0x0048ED2C,
    0x0048EE3B,
    0x004B045C,
    0x004B2EA8,
    0x004B2F34,
    0x004B314B,
    0x004B32AE,
    0x004BDAB3,
    0x004BF1E6,
    0x004CA26C,
    0x004CA2B4,
    0x004CA4EA,
    0x004CA539,
    0x004CE573,
    0x004CE685,
    0x004CF4E5,
    0x004CF740,
    0x004CF7F5,
};

std::vector<uintptr_t> fild_vsync_jp = {
    0x0022D505,
    0x0023424B,
    0x0023B6CF,
    0x00255706,
    0x002557C7,
    0x00255806,
    0x0030A1E3,
    0x003591AF,
    0x0039C2A2,
    0x003C2BD7,
    0x003C5109,
    0x003CB445,
    0x003CB8C0,
    0x003CCCBB,
    0x003CD533,
    0x003CD575,
    0x003D8478,
    0x003DDD5A,
    0x003E892C,
    0x003E9089,
    0x003E9978,
    0x003F12A3,
    0x00429590,
    0x0043B996,
    0x0043B9B6,
    0x0043EAD5,
    0x0043ED93,
    0x0043EE17,
    0x004433E5,
    0x00443455,
    0x00443539,
    0x00443575,
    0x00449E16,
    0x0044D2F6,
    0x00450400,
    0x00452D5C,
    0x0045FA46,
    0x0045FC3B,
    0x0046C7D2,
    0x0046DE95,
    0x0046F0A6,
    0x0046FBF0,
    0x00470A03,
    0x0047492E,
    0x00475899,
    0x00483B6A,
    0x00486CE7,
    0x0048E67C,
    0x0048E78B,
    0x004B0EDC,
    0x004B3928,
    0x004B39B4,
    0x004B3BCB,
    0x004B3D2E,
    0x004B5643,
    0x004C2E76,
    0x004CB9AC,
    0x004CB9F4,
    0x004CBC2A,
    0x004CBC79,
    0x004CE7E3,
    0x004CE8F5,
    0x004CF755,
    0x004CF9B0,
    0x004CFA65,
};

struct
{
    double frequency;
    double dt_target;
    double dt_sleep;
    double dt_multiplier;
    int32_t fps_target;
    int32_t fps_multiplier;
    int32_t *frame_counter;
    zmod::ini_map ini;
} globals;

void __stdcall game_wait_hook(LARGE_INTEGER *lpPerformanceCount, uint32_t vsync_30)
{
    auto dt_target = globals.dt_target;
    auto dt_sleep = globals.dt_sleep;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    auto dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;

    if (dt < dt_target)
    {
        while (dt < dt_sleep)
        {
            Sleep(1);
            QueryPerformanceCounter(&now);
            dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
        }
        while (dt < dt_target)
        {
            QueryPerformanceCounter(&now);
            dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
        }
        lpPerformanceCount->QuadPart = lpPerformanceCount->QuadPart + dt_target * globals.frequency; // Keep the timer from drifting
    }
    else
    {
        lpPerformanceCount->QuadPart = now.QuadPart; // Otherwise make sure we don't try to speed up the game
    }
}

void __stdcall timer_update_hook()
{
    auto t = globals.fps_target;
    if (t >= 120)
    {
        if (!(*globals.frame_counter % globals.fps_multiplier))
        {
            _timer_update();
        }
    }
    else if (t >= 90)
    {
        if (*globals.frame_counter % 3)
        {
            _timer_update();
        }
    }
    else
    {
        _timer_update();
    }
}

int __stdcall hud_state_update_hook()
{
    if (!(*globals.frame_counter % globals.fps_multiplier))
    {
        return _hud_state_update();
    }
    return 0;
}

intptr_t calculate_relative_offset(void *next_instruction, void *target)
{
    auto offset = (intptr_t)target - (intptr_t)next_instruction;
    return offset;
}

void set_timer_resolution()
{
    timeBeginPeriod(1);

    typedef NTSTATUS(CALLBACK * NtSetTimerResolution_t)(
        IN ULONG DesiredResolution,
        IN BOOLEAN SetResolution,
        OUT PULONG CurrentResolution);

    typedef NTSTATUS(CALLBACK * NtQueryTimerResolution_t)(
        OUT PULONG MaximumResolution,
        OUT PULONG MinimumResolution,
        OUT PULONG CurrentResolution);

    HMODULE hModule = LoadLibraryA("ntdll.dll");
    NtQueryTimerResolution_t NtQueryTimerResolution = (NtQueryTimerResolution_t)GetProcAddress(hModule, "NtQueryTimerResolution");
    NtSetTimerResolution_t NtSetTimerResolution = (NtSetTimerResolution_t)GetProcAddress(hModule, "NtSetTimerResolution");

    ULONG MaximumResolution, MinimumResolution, CurrentResolution;
    NtQueryTimerResolution(&MaximumResolution, &MinimumResolution, &CurrentResolution);
    NtSetTimerResolution(MinimumResolution, TRUE, &CurrentResolution);
}

void module_main(HINSTANCE instance)
{
    auto module_path = zmod::get_module_path(instance);
    auto ini_path = module_path.replace_extension(L".ini");
    auto module_key = module_path.stem().wstring();

    globals.ini = {
        {{module_key, L"process_priority_class"}, L"0x00000080"},
        {{module_key, L"target_fps"}, L"120.0"},
        {{module_key, L"clock_speed"}, L"1.0"},
    };

    if (!(zmod::file_exists(ini_path)))
    {
        zmod::write_ini_file(ini_path, globals.ini);
    }
    zmod::read_ini_file(ini_path, globals.ini);

    auto process_priority_class = std::stoul(globals.ini[{module_key, L"process_priority_class"}], nullptr, 0);
    if (process_priority_class)
    {
        if (!(SetPriorityClass(GetCurrentProcess(), process_priority_class)))
            ;
    }

    auto target_fps = std::wcstod(globals.ini[{module_key, L"target_fps"}].c_str(), nullptr);
    auto clock_speed = std::wcstod(globals.ini[{module_key, L"clock_speed"}].c_str(), nullptr);

    globals.fps_target = (int32_t)std::round(target_fps);
    globals.fps_multiplier = std::max((int32_t)std::round(target_fps) / 60, 1);
    globals.dt_target = 1.0 / (target_fps * clock_speed);
    globals.dt_sleep = globals.dt_target - 0.0016; // Sleep if we have greater than 1.6 ms to spare
    globals.dt_multiplier = 60.0 / std::round(target_fps);

    set_timer_resolution();

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    globals.frequency = (double)frequency.QuadPart;

    auto instruction = zmod::find_pattern("EB BF 84 1D");
    intptr_t offset = calculate_relative_offset((void *)instruction, game_wait_hook);
    zmod::write_memory((void *)(instruction - 4), &offset, 4);

    globals.frame_counter = *(int32_t **)(zmod::find_pattern("8B 1D ?? ?? ?? ?? B8 67") + 2);

    _timer_update = (function_554050_t *)(zmod::find_pattern("68 04 00 80 03 E8") - 10);
    _hud_state_update = (function_583320_t *)(zmod::find_pattern("55 56 57 33 DB 53") - 10);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID &)_timer_update, timer_update_hook);
    DetourAttach(&(PVOID &)_hud_state_update, hud_state_update_hook);
    DetourTransactionCommit();

    {
        auto p = &globals.fps_target;
        auto offscreen_move_address = zmod::find_pattern("8B 8A 90 3E 05 00 8B 2D");
        auto offscreen_move_patch = zmod::parse_hex("8B 0D ?? ?? ?? ?? 8B 82 90 3E 05 00 99 F7 F9 8B CA BD 01 00 00 00 31 FF EB 11");
        zmod::write_memory(&offscreen_move_patch.data()[2], &p, 4);
        zmod::write_memory((void *)offscreen_move_address, offscreen_move_patch.data(), offscreen_move_patch.size());

        auto offscreen_move_patch_2 = zmod::parse_hex("81 F9 ?? ?? ?? ?? 73 72 31 D2 EB 12");
        zmod::write_memory(&offscreen_move_patch_2.data()[2], &p, 4);
        zmod::write_memory((void *)(offscreen_move_address + 47), offscreen_move_patch_2.data(), offscreen_move_patch_2.size());

        auto offscreen_battle_address = zmod::find_pattern("8B 8A 90 3E 05 00 8B 35");
        auto offscreen_battle_patch = zmod::parse_hex("8B 0D ?? ?? ?? ?? 8B 82 90 3E 05 00 99 F7 F9 8B CA BE 01 00 00 00 31 FF 30 DB EB 11");
        zmod::write_memory(&offscreen_battle_patch.data()[2], &p, 4);
        zmod::write_memory((void *)offscreen_battle_address, offscreen_battle_patch.data(), offscreen_battle_patch.size());
    }

    {
        std::vector<uintptr_t> *fild_vsync;
        auto lang_id = (uint16_t *)(zmod::find_wstring(L"Translation") + 26);
        switch (*lang_id)
        {
        case 0x0404:
        case 0x0804:
            fild_vsync = &fild_vsync_zh;
            break;
        case 0x0411:
            fild_vsync = &fild_vsync_jp;
            break;
        default:
            fild_vsync = &fild_vsync_en;
            break;
        }

        auto p = &globals.dt_multiplier;
        auto base = zmod::get_base_address(nullptr);
        auto fld = zmod::parse_hex("DD 05 ?? ?? ?? ??");
        zmod::write_memory(&fld.data()[2], &p, 4);
        for (auto address : *fild_vsync)
        {
            zmod::write_memory((void *)(base + address), fld.data(), fld.size());
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;

        module_main(hinstDLL);
    }
    return TRUE;
}