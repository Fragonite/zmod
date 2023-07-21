#include <Windows.h>
#include <map>
#include <filesystem>
#include "detours.h"
#include "zmod_common.cpp"

std::filesystem::path module_path;

void load_dlls()
{
    auto parent_path = module_path.parent_path();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;
        module_path = zmod::get_module_path(hinstDLL);
        load_dlls();
        break;
    }
    return TRUE;
}