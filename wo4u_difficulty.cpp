#define NOMINMAX
#include <Windows.h>
#include <map>
#include <filesystem>
#include <stack>
#include "detours.h"
#include "zmod_common.cpp"

typedef void(fn_21BE00_t)(uint32_t map_id, uint32_t difficulty, uint32_t a3, uint32_t a4, uint8_t a5);
fn_21BE00_t initialise_map_hook; // Hook the function that sets the game difficulty

fn_21BE00_t *initialise_map_orig = nullptr;
struct
{
    void ****game_vtable = nullptr;
    void **game_info = nullptr;
    double party_level_multiplier;
} globals;

void initialise_map_hook(uint32_t map_id, uint32_t difficulty, uint32_t a3, uint32_t a4, uint8_t a5)
{
    enum
    {
        EASY = 0,
        NORMAL = 1,
        HARD = 2,
        CHAOTIC = 3,
        PANDEMONIUM = 4,
    };

    difficulty = PANDEMONIUM;
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
    auto party_ids = (uint16_t *)(&(*(uint8_t **)globals.game_info)[0xF70]);
    auto party_level_multiplier = globals.party_level_multiplier;
    double first, second, third;

    auto officer = get_officer_data(vt, party_ids[0]);
    first = (officer[OFFICER_PROMOTION]) ? 99.0 : (double)officer[OFFICER_LEVEL];

    officer = get_officer_data(vt, party_ids[1]);
    second = (officer[OFFICER_PROMOTION]) ? 99.0 : (double)officer[OFFICER_LEVEL];

    officer = get_officer_data(vt, party_ids[2]);
    third = (officer[OFFICER_PROMOTION]) ? 99.0 : (double)officer[OFFICER_LEVEL];

    return std::round((first + second + third) * party_level_multiplier / 3.0);
}

uint32_t calculate_capped_new_map_difficulty()
{
    return std::min((uint32_t)0x2D, calculate_new_map_difficulty());
}

void insert_relative_address(void *dst, void *next_instruction, void *absolute)
{
    auto relative = (intptr_t)absolute - (intptr_t)next_instruction;
    memcpy(dst, &relative, 4);
}

void insert_relative_address(void *dst, void *abs)
{
    auto relative = (intptr_t)abs - ((intptr_t)dst + 4);
    memcpy(dst, &relative, 4);
}

void module_main(HINSTANCE hinstDLL)
{
    zmod::ini ini = {
        {{L"zmod_wo4u_difficulty", L"perpetual_pandemonium"}, L"0"},
        {{L"zmod_wo4u_difficulty", L"scale_sorties_to_party_level"}, L"0"},
        {{L"zmod_wo4u_difficulty", L"scale_new_weapons_to_party_level"}, L"0"},
        {{L"zmod_wo4u_difficulty", L"scale_lottery_weapons_to_party_level"}, L"0"},
        {{L"zmod_wo4u_difficulty", L"update_recommended_lv_ui"}, L"0"},
        {{L"zmod_wo4u_difficulty", L"party_level_multiplier"}, L"1.0"},
    };

    auto ini_path = zmod::get_module_path(hinstDLL).replace_filename(L"zmod_wo4u_difficulty.ini");
    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    zmod::memory mem;
    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    globals.game_vtable = (void ****)(wo4u + 0xF441C0);
    globals.game_info = (void **)(wo4u + 0xF441E8);
    globals.party_level_multiplier = std::wcstod(ini[{L"zmod_wo4u_difficulty", L"party_level_multiplier"}].c_str(), nullptr);

    if (ini[{L"zmod_wo4u_difficulty", L"perpetual_pandemonium"}] != L"0")
    {
        initialise_map_orig = (fn_21BE00_t *)(zmod::find_pattern(wo4u, 0xFFFFFF, "45 33 ED 81 F9 2B 01 00 00") - 0x3F);
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)initialise_map_orig, initialise_map_hook);
        DetourTransactionCommit();
    }

    if (ini[{L"zmod_wo4u_difficulty", L"scale_sorties_to_party_level"}] != L"0")
    {

        // Scale sorties
        {
            auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "41 0F BE 87 EE 04 00 00");
            auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? 44 8B E0");
            insert_relative_address((void *)&patch[1], (void *)&addr[5], calculate_new_map_difficulty);
            mem.write((void *)addr, patch.data(), patch.size());
        }

        if (ini[{L"zmod_wo4u_difficulty", L"update_recommended_lv_ui"}] != L"0")
        {
            // Sortie selection screen
            {
                auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "0F 45 C8 BA 63 00 00 00");
                auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? 8B D0 49 8D 9E 50 01 00 00 EB 04");
                mem.write((void *)addr, patch.data(), patch.size());
                insert_relative_address((void *)&addr[1], calculate_new_map_difficulty);
            }
            // Main screen
            {
                auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "BA 63 00 00 00 0F B6 48 05");
                auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? 8B D0 EB 08");
                mem.write((void *)addr, patch.data(), patch.size());
                insert_relative_address((void *)&addr[1], calculate_new_map_difficulty);
            }
        }
    }

    // Scale new weapons
    if (ini[{L"zmod_wo4u_difficulty", L"scale_new_weapons_to_party_level"}] != L"0")
    {
        auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "0F BE 48 04 B8 FF FF FF FF");
        auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? EB 08");
        mem.write((void *)addr, patch.data(), patch.size());
        insert_relative_address((void *)&addr[1], calculate_capped_new_map_difficulty);
    }

    // Scale lottery weapons
    if (ini[{L"zmod_wo4u_difficulty", L"scale_lottery_weapons_to_party_level"}] != L"0")
    {
        auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "0F BE 48 04 8B C6");
        auto patch = zmod::parse_hex("E8 ?? ?? ?? ?? 8B C8 EB 03");
        mem.write((void *)addr, patch.data(), patch.size());
        insert_relative_address((void *)&addr[1], calculate_capped_new_map_difficulty);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;
        if (zmod::get_module_path(NULL).filename().wstring().find(L"WO4.exe") == std::wstring::npos)
        {
            return FALSE;
        }
        module_main(hinstDLL);
        break;
    }
    return TRUE;
}