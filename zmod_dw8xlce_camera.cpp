#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <filesystem>
#include <map>
#include <vector>
#include <sstream>
#include <numbers>
#include "zmod_common.cpp"

namespace camera
{

    struct camera_settings
    {
        struct camera
        {
            float min_distance;
            float max_distance;
            float height;
            float UNKNOWN_1;
            float angle;
            float UNKNOWN_2;
            float UNKNOWN_3;
        };
        camera standard;
        camera blocking;
    };

    void module_main(HINSTANCE instance)
    {
        auto camera = (camera_settings *)(zmod::find_pattern("E1 43 00 00 FA 43") - 2);

        auto module_path = zmod::get_module_path(instance);
        auto ini_path = module_path.replace_extension(L".ini");

        zmod::ini_map ini = {
            {{L"standard_camera", L"min_distance"}, L"450.0"},
            {{L"standard_camera", L"max_distance"}, L"500.0"},
            {{L"standard_camera", L"height"}, L"130.0"},
            {{L"standard_camera", L"angle"}, L"14.0"},
            {{L"blocking_camera", L"min_distance"}, L"450.0"},
            {{L"blocking_camera", L"max_distance"}, L"500.0"},
            {{L"blocking_camera", L"height"}, L"130.0"},
            {{L"blocking_camera", L"angle"}, L"0.0"},
        };

        if (!(zmod::file_exists(ini_path)))
        {
            zmod::write_ini_file(ini_path, ini);
        }
        zmod::read_ini_file(ini_path, ini);

        camera->standard.min_distance = std::wcstof(ini[{L"standard_camera", L"min_distance"}].c_str(), nullptr);
        camera->standard.max_distance = std::wcstof(ini[{L"standard_camera", L"max_distance"}].c_str(), nullptr);
        camera->standard.height = std::wcstof(ini[{L"standard_camera", L"height"}].c_str(), nullptr);
        camera->standard.angle = (-(std::wcstof(ini[{L"standard_camera", L"angle"}].c_str(), nullptr))) * (std::numbers::pi / 180.0);

        camera->blocking.min_distance = std::wcstof(ini[{L"blocking_camera", L"min_distance"}].c_str(), nullptr);
        camera->blocking.max_distance = std::wcstof(ini[{L"blocking_camera", L"max_distance"}].c_str(), nullptr);
        camera->blocking.height = std::wcstof(ini[{L"blocking_camera", L"height"}].c_str(), nullptr);
        camera->blocking.angle = (-(std::wcstof(ini[{L"blocking_camera", L"angle"}].c_str(), nullptr))) * (std::numbers::pi / 180.0);
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
}