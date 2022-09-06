#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>
#include <numbers>
#include <map>
#include "io_helper.cpp"

struct camera_settings
{
    struct camera
    {
        float minDistance;
        float maxDistance;
        float height;
        float UNKNOWN01;
        float angle;
        float UNKNOWN02;
        float UNKNOWN03;
    };
    camera standard;
    camera blocking;
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;

        ini_map iniMap = {
            {{L"standard_camera", L"min_distance"}, L"450.0"},
            {{L"standard_camera", L"max_distance"}, L"500.0"},
            {{L"standard_camera", L"height"}, L"130.0"},
            {{L"standard_camera", L"angle"}, L"14.0"},

            {{L"blocking_camera", L"min_distance"}, L"450.0"},
            {{L"blocking_camera", L"max_distance"}, L"500.0"},
            {{L"blocking_camera", L"height"}, L"130.0"},
            {{L"blocking_camera", L"angle"}, L"0.0"},
        };

        auto iniPath = get_module_path(hinstDLL).replace_extension(L".ini");
        if (!(file_exists(iniPath)))
        {
            create_ini_file(iniPath, iniMap);
        }
        read_ini_file(iniPath, iniMap);

        auto base = (uint32_t)GetModuleHandleW(nullptr);
        auto camera = (camera_settings *)(base + 0x662BA0);
        camera->standard.minDistance = std::wcstof(iniMap[{L"standard_camera", L"min_distance"}].c_str(), nullptr);
        camera->standard.maxDistance = std::wcstof(iniMap[{L"standard_camera", L"max_distance"}].c_str(), nullptr);
        camera->standard.height = std::wcstof(iniMap[{L"standard_camera", L"height"}].c_str(), nullptr);
        camera->standard.angle = (-(std::wcstof(iniMap[{L"standard_camera", L"angle"}].c_str(), nullptr))) * (std::numbers::pi / 180.0);

        camera->blocking.minDistance = std::wcstof(iniMap[{L"blocking_camera", L"min_distance"}].c_str(), nullptr);
        camera->blocking.maxDistance = std::wcstof(iniMap[{L"blocking_camera", L"max_distance"}].c_str(), nullptr);
        camera->blocking.height = std::wcstof(iniMap[{L"blocking_camera", L"height"}].c_str(), nullptr);
        camera->blocking.angle = (-(std::wcstof(iniMap[{L"blocking_camera", L"angle"}].c_str(), nullptr))) * (std::numbers::pi / 180.0);
        break;
    // case DLL_PROCESS_DETACH:
    //     break;
    }
    return TRUE;
}