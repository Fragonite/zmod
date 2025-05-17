#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <map>
#include <filesystem>
#include <thread>
#include "detours.h"
#include "zmod_common.cpp"

namespace bg3
{
    void speedhack_toggle_listener(float global_speed_multiplier)
    {
        float *game_speed = nullptr;

        do
        {
            game_speed = (float *)(zmod::find_pattern_in_heap("66 66 a6 3f 00 00 80 3f 9a 99 99 3e") + 4);
        } while (game_speed == nullptr);

        *game_speed = global_speed_multiplier;

        auto is_speedhack_enabled = true;
        while (true)
        {
            if (GetAsyncKeyState(VK_F24) & 0x0001)
            {
                is_speedhack_enabled = !is_speedhack_enabled;
                *game_speed = (is_speedhack_enabled) ? global_speed_multiplier : 1.0;
                Sleep(250);
            }
            else
            {
                Sleep(25);
            }
        }
    }

    void module_main(HINSTANCE hinstDLL)
    {
        auto ini = zmod::ini(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_bg3.ini"));
        ini.set_many({
            {{L"zmod_bg3", L"global_speed_multiplier"}, L"1.0"},
            {{L"zmod_bg3", L"process_priority_class"}, L"0x00000080"},
        });
        if (ini.exists())
        {
            ini.load();
        }
        else
        {
            ini.save();
        }

        auto global_speed_multiplier = ini.get_float({L"zmod_bg3", L"global_speed_multiplier"});

        zmod::set_timer_resolution();

        auto process_priority_class = ini.get_uint({L"zmod_bg3", L"process_priority_class"}, 0);
        if (process_priority_class)
        {
            if (!(SetPriorityClass(GetCurrentProcess(), process_priority_class)))
                ;
        }

        std::thread t(speedhack_toggle_listener, global_speed_multiplier);
        t.detach();
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        bg3::module_main(hinstDll);
        break;
    }
    return TRUE;
}