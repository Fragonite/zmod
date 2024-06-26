#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <dwmapi.h>
#include <map>
#include <filesystem>
#include <stack>
#include <numbers>
#include <numeric>
#include <fstream>
#include <ostream>
#include <sstream>
#include <iostream>
#include "detours.h"
#include "zmod_common.cpp"

#define DEBUG(...)                                                                                   \
    do                                                                                               \
    {                                                                                                \
        std::ostringstream os;                                                                       \
        os << "DEBUG: " << (__VA_ARGS__);                                                            \
        os << " (FILE: " << __FILE__ << ", LINE: " << __LINE__ << ", FUNCTION: " << __func__ << ")"; \
        std::cerr << os.str() << std::endl;                                                          \
    } while (0)

typedef void(fn_7024D0_t)(LARGE_INTEGER *lpPerformanceCount, uint64_t vsync_30);
fn_7024D0_t wait_loop;

typedef void(fn_21BE00_t)(uint32_t map_id, uint32_t difficulty, uint32_t a3, uint32_t a4, uint8_t a5);
fn_21BE00_t initialise_map_hook;
fn_21BE00_t *initialise_map_orig = nullptr;

struct
{
    std::ofstream log;

    double frequency;
    double target_fps;
    double dt_target;
    double dt_sleep;
    double late_frame_compensation_multiplier;

    void ****game_vtable = nullptr;
    void **game_info = nullptr;
    double party_level_multiplier;
    uint32_t difficulty;
} globals;

const uint8_t *find_pattern(const std::string &pattern)
{
    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    return zmod::find_pattern(wo4u, 0xFFFFFF, pattern);
}

void initialise_map_hook(uint32_t map_id, uint32_t difficulty, uint32_t a3, uint32_t a4, uint8_t a5)
{
    difficulty = globals.difficulty;
    initialise_map_orig(map_id, difficulty, a3, a4, a5);
}

uint8_t *get_officer_data(void ***vtable, uint64_t officer_id)
{
    typedef uint8_t *(fn_2406F0_t)(void ***vtable, uint64_t officer_id);
    return (*(fn_2406F0_t ***)vtable)[2](vtable, officer_id);
}

uint32_t calculate_new_map_difficulty()
{
    enum
    {
        OFFICER_PROMOTION = 0x177,
        OFFICER_LEVEL = 0x378,
    };

    auto vt = *globals.game_vtable;
    // Yeah...
    auto party_ids = (uint16_t *)(&(*(uint8_t **)globals.game_info)[0xF70]);
    double character_levels[3] = {};

    // Level 1 officers have a level of 0, so we need to add 1 to the level to correct for the party level multiplier, then subtract 1.
    for (auto i = 0; i < 3; i++)
    {
        auto officer = get_officer_data(vt, party_ids[i]);
        character_levels[i] = (officer[OFFICER_PROMOTION]) ? 100.0 : (double)officer[OFFICER_LEVEL] + 1.0;
    }

    auto sum = std::accumulate(std::begin(character_levels), std::end(character_levels), 0.0);
    return std::round(sum * globals.party_level_multiplier / 3.0 - 1.0);
}

uint32_t calculate_capped_new_map_difficulty()
{
    return std::min((uint32_t)0x2D, calculate_new_map_difficulty());
}

void wait_loop(LARGE_INTEGER *lpPerformanceCount, uint64_t vsync_30)
{
    auto dt_target = globals.dt_target;
    auto dt_sleep = globals.dt_sleep;
    auto lfcm = globals.late_frame_compensation_multiplier;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    auto dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
    if (dt < dt_target * lfcm)
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
        lpPerformanceCount->QuadPart = lpPerformanceCount->QuadPart + (LONGLONG)(dt_target * globals.frequency); // Prevent drift
    }
    else
    {
        lpPerformanceCount->QuadPart = now.QuadPart; // Prevent the game from speeding up if dt >= dt_target * lfcm
    }
}

void setup_wait_loop_hook()
{
    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "48 8B 8F 08 18 00 00") - 5;
    zmod::setup_call(addr, wait_loop);
}

void setup_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C9 48 8B C8");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 0D 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    zmod::setup_jmp(jmp_site, code);
}

void setup_camera_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 F3 0F 11 46 10");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    zmod::setup_jmp(jmp_site, code);
}

void setup_musou_camera_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 F3 0F 11 84 8B 78 01 00 00");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    zmod::setup_jmp(jmp_site, code);
}

void setup_unity_magic_camera_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 F3 41 0F 11 84 9D 78 01 00 00");
    if (jmp)
    {
        auto speed = (float)(60.0 / globals.target_fps);
        auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp + 5), speed);
        zmod::write_memory_unsafe(code, patch.data(), patch.size());
        zmod::setup_jmp(jmp, code);
    }
    else
    {
        DEBUG((intptr_t)jmp);
    }
}

void setup_physics_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A F0 F3 0F 59 35 ?? ?? ?? ?? 48 83 7F 48 00");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 35 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    zmod::setup_jmp(jmp_site, code);
}

// Particle lifetime is based on frame rate. This hook only adjusts particle and effect speed.
void setup_effects_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 8B 01");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    zmod::setup_jmp(jmp_site, code);
}

void setup_rim_lighting_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 0F 58 83 D8 34 00 00") - 13;
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    zmod::setup_jmp(jmp_site, code);
}

void setup_block_cancel_hook(float multiplier)
{
    auto jmp = find_pattern("F3 0F 10 8B 2C 01 00 00 0F 2F CE 76 2D") + 22;
    if (jmp)
    {
        static const uint8_t code[64] = {};
        zmod::unprotect(code, sizeof(code));

        auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", zmod::rel(code + 13, jmp + 5), multiplier);
        zmod::write_memory_unsafe(code, patch.data(), patch.size());
        zmod::setup_jmp(jmp, code);
    }
    else
    {
        DEBUG((intptr_t)jmp);
    }
}

void module_main(HINSTANCE hinstDLL)
{
    // Save the original buffers.
    std::streambuf *coutbuf = std::cout.rdbuf();
    std::streambuf *cerrbuf = std::cerr.rdbuf();

    // Redirect cout and cerr to the log file.
    globals.log.open(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_wo4u.log"));
    std::cout.rdbuf(globals.log.rdbuf());
    std::cerr.rdbuf(globals.log.rdbuf());

    DEBUG("Hello there!");

    auto ini = zmod::ini(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_wo4u.ini"));
    ini.set_many({
        {{L"config", L"enable_performance_mod"}, L"1"},
        {{L"config", L"enable_camera_mod"}, L"1"},
        {{L"config", L"enable_difficulty_mod"}, L"0"},
        {{L"config", L"enable_gameplay_mod"}, L"0"},

        {{L"performance", L"global_speed_multiplier"}, L"1.0"},
        {{L"performance", L"late_frame_compensation_multiplier"}, L"1.0"},
        {{L"performance", L"process_priority_class"}, L"0x00000080"},
        {{L"performance", L"target_fps"}, L"auto"},

        {{L"camera", L"angle"}, L"15.4"},
        {{L"camera", L"max_distance"}, L"560.0"},
        {{L"camera", L"min_distance"}, L"510.0"},

        {{L"difficulty", L"force_chaotic"}, L"1"},
        {{L"difficulty", L"force_pandemonium"}, L"1"},
        {{L"difficulty", L"party_level_multiplier"}, L"1.0"},
        {{L"difficulty", L"scale_lottery_weapons_to_party_level"}, L"1"},
        {{L"difficulty", L"scale_new_weapons_to_party_level"}, L"1"},
        {{L"difficulty", L"scale_sorties_to_party_level"}, L"1"},
        {{L"difficulty", L"update_recommended_lv_ui"}, L"1"},

        {{L"gameplay", L"block_cancel_delay_multiplier"}, L"1.0"},
        {{L"gameplay", L"block_cancel_for_everyone"}, L"1"},
        {{L"gameplay", L"open_all_gates"}, L"1"},
        {{L"gameplay", L"temporarily_unlock_all_characters"}, L"0"},
        {{L"gameplay", L"mount_speed"}, L"auto"},
    });
    if (ini.exists())
    {
        ini.load();
    }
    else
    {
        ini.save();
    }

    if (ini.get_wstring({L"config", L"enable_performance_mod"}) != L"0")
    {
        auto process_priority_class = ini.get_uint({L"performance", L"process_priority_class"});
        if (process_priority_class)
        {
            DEBUG(SetPriorityClass(GetCurrentProcess(), process_priority_class));
        }

        zmod::set_timer_resolution();
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        globals.frequency = (double)frequency.QuadPart;

        auto target_fps = ini.get_double({L"performance", L"target_fps"});
        auto dt_target = 1.0 / (target_fps * ini.get_double({L"performance", L"global_speed_multiplier"}));

        if (zmod::to_lower(ini.get_wstring({L"performance", L"target_fps"})) == L"auto")
        {
            // Get high resolution timing info.
            DWM_TIMING_INFO timingInfo = {sizeof(DWM_TIMING_INFO)};
            if (DwmGetCompositionTimingInfo(NULL, &timingInfo) == S_OK)
            {
                DEBUG("DwmGetCompositionTimingInfo may prevent clean shutdown.");
                target_fps = (double)timingInfo.rateRefresh.uiNumerator / (double)timingInfo.rateRefresh.uiDenominator;
            }
            else
            {
                // Get rounded refresh rate as a fallback.
                DEVMODEW devMode = {0};
                devMode.dmSize = sizeof(devMode);

                if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devMode))
                {
                    target_fps = (double)devMode.dmDisplayFrequency;
                }
            }
        }

        DEBUG("Target FPS: ", target_fps);
        globals.target_fps = target_fps;
        globals.dt_target = dt_target;
        globals.dt_sleep = dt_target - 0.0016;
        globals.late_frame_compensation_multiplier = std::max(1.0, ini.get_double({L"performance", L"late_frame_compensation_multiplier"}));

        setup_wait_loop_hook();
        setup_speed_hook();
        setup_camera_speed_hook();
        setup_musou_camera_speed_hook();
        setup_unity_magic_camera_speed_hook();
        setup_physics_speed_hook();
        setup_effects_speed_hook();
        setup_rim_lighting_hook();
    }

    if (ini.get_wstring({L"config", L"enable_camera_mod"}) != L"0")
    {
        struct camera
        {
            float min_distance;
            float max_distance;
            float UNKNOWN_PI;
            float UNKNOWN_ONE_1;
            float UNKNOWN_ZERO;
            float height;
            float UNKNOWN_FIFTEEN;
            float UNKNOWN_ONE_2;
            float angle;
        };

        auto wo4u = zmod::get_base_address(L"WO4U.dll");
        auto cam = (camera *)zmod::find_pattern(wo4u, 0xFFFFFF, "00 00 FF 43 00 00 0C 44");

        cam->min_distance = ini.get_float({L"camera", L"min_distance"});
        cam->max_distance = ini.get_float({L"camera", L"max_distance"});
        cam->angle = (-(ini.get_float({L"camera", L"angle"}))) * (std::numbers::pi / 180.0);
    }

    if (ini.get_wstring({L"config", L"enable_difficulty_mod"}) != L"0")
    {
        auto wo4u = zmod::get_base_address(L"WO4U.dll");
        globals.game_vtable = (void ****)(wo4u + 0xF441C0);
        globals.game_info = (void **)(wo4u + 0xF441E8);
        globals.party_level_multiplier = ini.get_double({L"difficulty", L"party_level_multiplier"});

        uint32_t difficulty = 0;
        if (ini.get_wstring({L"difficulty", L"force_chaotic"}) != L"0")
        {
            difficulty = 3;
        }
        if (ini.get_wstring({L"difficulty", L"force_pandemonium"}) != L"0")
        {
            difficulty = 4;
        }
        if (difficulty)
        {
            globals.difficulty = difficulty;
            initialise_map_orig = (fn_21BE00_t *)(zmod::find_pattern(wo4u, 0xFFFFFF, "45 33 ED 81 F9 2B 01 00 00") - 0x3F);
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID &)initialise_map_orig, initialise_map_hook);
            DetourTransactionCommit();
        }

        if (ini.get_wstring({L"difficulty", L"scale_sorties_to_party_level"}) != L"0")
        {

            // Scale sorties.
            {
                auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "41 0F BE 87 EE 04 00 00");
                auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? 44 8B E0", zmod::rel(addr + 5, calculate_new_map_difficulty));
                zmod::write_memory(addr, patch.data(), patch.size());
            }

            if (ini.get_wstring({L"difficulty", L"update_recommended_lv_ui"}) != L"0")
            {
                // Sortie selection screen.
                {
                    auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "0F 45 C8 BA 63 00 00 00");
                    auto patch = zmod::parse_hex("E8 .... 8B D0 49 8D 9E 50 01 00 00 EB 04", zmod::rel(addr + 5, calculate_new_map_difficulty));
                    zmod::write_memory(addr, patch.data(), patch.size());
                }
                // Main screen.
                {
                    auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "BA 63 00 00 00 0F B6 48 05");
                    auto patch = zmod::parse_hex("E8 .... 8B D0 EB 08", zmod::rel(addr + 5, calculate_new_map_difficulty));
                    zmod::write_memory(addr, patch.data(), patch.size());
                }
            }
        }

        // Scale new weapons.
        if (ini.get_wstring({L"difficulty", L"scale_new_weapons_to_party_level"}) != L"0")
        {
            auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "0F BE 48 04 B8 FF FF FF FF");
            auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? EB 08", zmod::rel(addr + 5, calculate_capped_new_map_difficulty));
            zmod::write_memory(addr, patch.data(), patch.size());
        }

        // Scale lottery weapons.
        if (ini.get_wstring({L"difficulty", L"scale_lottery_weapons_to_party_level"}) != L"0")
        {
            auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "0F BE 48 04 8B C6");
            auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? 8B C8 EB 03", zmod::rel(addr + 5, calculate_capped_new_map_difficulty));
            zmod::write_memory(addr, patch.data(), patch.size());
        }
    }

    if (ini.get_wstring({L"config", L"enable_gameplay_mod"}) != L"0")
    {
        // TODO: Implement block cancel for technique and speed characters separately.
        if (ini.get_wstring({L"gameplay", L"block_cancel_for_everyone"}) != L"0")
        {
            // Patch block cancel code that checks for power type character.
            {
                auto addr = find_pattern("0F BE 48 02 83 F9 02 0F 46 F9 85 FF 0F 85 E2 00 00 00");
                DEBUG((intptr_t)addr);
                // xor ecx,ecx
                // nop 2
                auto patch = zmod::parse_hex("31 C9 66 90");
                zmod::write_memory(addr, patch.data(), patch.size());
            }

            // Patch character type check - all character types pass power type check.
            {
                auto addr = find_pattern("E8 E5 15 00 00 E9 C8 03 00 00");
                DEBUG((intptr_t)addr);
                // mov al,01
                // nop 3
                auto patch = zmod::parse_hex("B0 01 0F 1F 00");
                zmod::write_memory(addr, patch.data(), patch.size());
            }
        }

        if (ini.get_wstring({L"gameplay", L"open_all_gates"}) != L"0")
        {
            auto addr = find_pattern("75 0D B0 01 48 8B 5C 24 30 48 83 C4 20 5F C3 83 FF 05");
            DEBUG((intptr_t)addr);
            auto patch = zmod::parse_hex("66 90");
            zmod::write_memory(addr, patch.data(), patch.size());
        }

        if (ini.get_wstring({L"gameplay", L"temporarily_unlock_all_characters"}) != L"0")
        {
            // Found in a public cheat table, thanks nakint!
            auto addr = find_pattern("41 8B 84 91 A4 0F 00 00 0F A3 C8 41 0F 92 C7");
            DEBUG((intptr_t)addr);
            auto patch = zmod::parse_hex("41 B7 01 E9 07 00 00 00");
            zmod::write_memory(addr, patch.data(), patch.size());
        }

        if (ini.get_wstring({L"gameplay", L"mount_speed"}) != L"0")
        {
            if (zmod::to_lower(ini.get_wstring({L"gameplay", L"mount_speed"})) != L"auto")
            {
                auto addr = find_pattern("3B D0 0F 42 C2 F3 48 0F 2A C0 F3 0F 59 05 ?? ?? ?? ?? F3 0F 59 C6");
                DEBUG((intptr_t)addr);
                auto patch = zmod::parse_hex("B8 ....", ini.get_uint({L"gameplay", L"mount_speed"}));
                zmod::write_memory(addr, patch.data(), patch.size());
            }
        }
    }

    if (ini.get_wstring({L"config", L"enable_performance_mod"}) != L"0" || ini.get_wstring({L"config", L"enable_gameplay_mod"}) != L"0")
    {
        auto delta = 1.0;
        if (ini.get_wstring({L"config", L"enable_performance_mod"}) != L"0")
        {
            delta = 60.0 / globals.target_fps;
        }
        if (ini.get_wstring({L"config", L"enable_gameplay_mod"}) != L"0")
        {
            delta /= ini.get_double({L"gameplay", L"block_cancel_delay_multiplier"});
        }
        setup_block_cancel_hook(delta);
    }

    // Restore cout and cerr to their original state.
    std::cout.rdbuf(coutbuf);
    std::cerr.rdbuf(cerrbuf);

    // Close the log file.
    globals.log.close();
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        if (zmod::get_module_path(NULL).filename().wstring().find(L"WO4.exe") == std::wstring::npos)
        {
            return FALSE;
        }
        module_main(hinstDll);
        break;
    }
    return TRUE;
}