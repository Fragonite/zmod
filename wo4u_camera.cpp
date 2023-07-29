#include <Windows.h>
#include <map>
#include <filesystem>
#include <numbers>
#include "zmod_common.cpp"

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

void module_main(HINSTANCE hinstDLL)
{
    // 150.0 is already optimal for camera height and other values cause problems when jumping
    // All values are default/from the game
    zmod::ini ini = {
        {{L"zmod_wo4u_camera", L"min_distance"}, L"510.0"},
        {{L"zmod_wo4u_camera", L"max_distance"}, L"560.0"},
        // {{L"zmod_wo4u_camera", L"height"}, L"150.0"},
        {{L"zmod_wo4u_camera", L"angle"}, L"15.4"},
    };

    auto module_path = zmod::get_module_path(hinstDLL);
    auto ini_path = module_path.replace_filename(L"zmod_wo4u_camera.ini");

    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto cam = (camera *)zmod::find_pattern(wo4u, 0xFFFFFF, "00 00 FF 43 00 00 0C 44");

    cam->min_distance = std::wcstof(ini[{L"zmod_wo4u_camera", L"min_distance"}].c_str(), nullptr);
    cam->max_distance = std::wcstof(ini[{L"zmod_wo4u_camera", L"max_distance"}].c_str(), nullptr);
    cam->height = std::wcstof(ini[{L"zmod_wo4u_camera", L"height"}].c_str(), nullptr);
    cam->angle = (-(std::wcstof(ini[{L"zmod_wo4u_camera", L"angle"}].c_str(), nullptr))) * (std::numbers::pi / 180.0);
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