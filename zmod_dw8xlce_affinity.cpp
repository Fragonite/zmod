#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
#include <map>
#include "zmod_common.cpp"

void module_main(HINSTANCE instance)
{
    auto module_path = zmod::get_module_path(instance);
    auto ini_path = module_path.replace_filename(L"zmod_dw8xlce_affinity.ini");

    zmod::ini ini = {
        {{L"zmod_dw8xlce_affinity", L"force_neutral_affinity"}, L"0"},
    };

    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    if (ini[{L"zmod_dw8xlce_affinity", L"force_neutral_affinity"}] != L"0")
    {
        auto addr = zmod::find_pattern("8B 44 94 04 33 CC");
        auto patch = zmod::parse_hex("31 C0 40 90");
        zmod::write_memory((void *)addr, patch.data(), patch.size());
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;
        module_main(hinstDLL);
        break;
    }
    return TRUE;
}