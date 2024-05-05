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

    void set_ini_defaults(zmod::ini &ini)
    {
        ini.set_many({
            {{L"camera", L"min_distance"}, L"450.0"},
            {{L"camera", L"max_distance"}, L"500.0"},
            {{L"camera", L"height"}, L"130.0"},
            {{L"camera", L"angle"}, L"14.0"},
            {{L"camera", L"blocking_min_distance"}, L"450.0"},
            {{L"camera", L"blocking_max_distance"}, L"500.0"},
            {{L"camera", L"blocking_height"}, L"130.0"},
            {{L"camera", L"blocking_angle"}, L"0.0"},
        });
    }

    void module_main(zmod::ini &ini)
    {
        auto camera = (camera_settings *)(zmod::find_pattern("E1 43 00 00 FA 43") - 2);

        camera->standard.min_distance = ini.get_float({L"camera", L"min_distance"});
        camera->standard.max_distance = ini.get_float({L"camera", L"max_distance"});
        camera->standard.height = ini.get_float({L"camera", L"height"});
        camera->standard.angle = (-(ini.get_float({L"camera", L"angle"}))) * (std::numbers::pi / 180.0);

        camera->blocking.min_distance = ini.get_float({L"camera", L"blocking_min_distance"});
        camera->blocking.max_distance = ini.get_float({L"camera", L"blocking_max_distance"});
        camera->blocking.height = ini.get_float({L"camera", L"blocking_height"});
        camera->blocking.angle = (-(ini.get_float({L"camera", L"blocking_angle"}))) * (std::numbers::pi / 180.0);
    }
}