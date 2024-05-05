#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <dwmapi.h>
#include <filesystem>
#include <map>
#include <numbers>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include "detours.h"
#include "zmod_common.cpp"
#include "zmod_dw8xlce_frame_time.cpp"
#include "zmod_dw8xlce_ultrawide.cpp"

void module_main(HINSTANCE hinstDLL)
{
    auto ini = zmod::ini(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_dw8xlce.ini"));
    ini.set_many({
        {{L"config", L"enable_frame_time_mod"}, L"1"},
        {{L"config", L"enable_ultrawide_mod"}, L"1"},
        {{L"config", L"enable_camera_mod"}, L"1"},
        {{L"config", L"enable_affinity_mod"}, L"0"},
    });
    if (ini.exists())
    {
        ini.load();
    }
    else
    {
        frame_time::set_ini_defaults(ini);
        ultrawide::set_ini_defaults(ini);
        ini.save();
    }

    if (ini.get_bool({L"config", L"enable_frame_time_mod"}))
    {
        frame_time::module_main(ini);
        ultrawide::module_main(ini);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        if (zmod::get_module_path(NULL).filename().wstring().find(L"Launch.exe") == std::wstring::npos)
        {
            return FALSE;
        }
        module_main(hinstDll);
        break;
    }
    return TRUE;
}