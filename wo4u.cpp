#include <Windows.h>
#include <map>
#include <filesystem>
#include "detours.h"
#include "zmod_common.cpp"

typedef void(fn_21BE00_t)(uint32_t map_id, uint32_t difficulty, uint32_t a3, uint32_t a4, uint8_t a5);
fn_21BE00_t initialise_map_hook;
fn_21BE00_t *initialise_map_orig = nullptr;

void initialise_map_hook(uint32_t map_id, uint32_t difficulty, uint32_t a3, uint32_t a4, uint8_t a5)
{
    difficulty = 4;
    initialise_map_orig(map_id, difficulty, a3, a4, a5);
}

void module_main(HINSTANCE hinstDLL)
{
    zmod::ini ini = {
        {{L"zmod_wo4u_difficulty", L"perpetual_pandemonium"}, L"0"},
        {{L"zmod_wo4u_difficulty", L"scale_sorties_to_party_level"}, L"0"},
    };

    auto module_path = zmod::get_module_path(hinstDLL);
    auto ini_path = module_path.replace_filename(L"zmod_wo4u_difficulty.ini");

    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    if (ini[{L"difficulty", L"perpetual_pandemonium"}] != L"0")
    {
        initialise_map_orig = (fn_21BE00_t *)(zmod::find_pattern("45 33 ED 81 F9 2B 01 00 00") - 0x3F);
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)initialise_map_orig, initialise_map_hook);
        DetourTransactionCommit();
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
        break;
    }
    return TRUE;
}