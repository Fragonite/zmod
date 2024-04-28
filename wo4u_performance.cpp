#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <map>
#include <filesystem>
#include <stack>
#include "zmod_common.cpp"

typedef void(fn_7024D0_t)(LARGE_INTEGER *lpPerformanceCount, uint64_t vsync_30);
fn_7024D0_t wait_loop;

struct
{
    double frequency;
    double target_fps;
    double dt_target;
    double dt_sleep;
    double late_frame_compensation_multiplier;
} globals;

template <typename T, typename U>
int32_t rel(T *next_instruction, U *absolute)
{
    return (int32_t)((intptr_t)absolute - (intptr_t)next_instruction);
}

template <typename T, typename U>
void setup_jmp(T *pounce, U *splat)
{
    auto jmp = zmod::parse_hex("E9 ?? ?? ?? ??", rel(pounce + 5, splat));
    zmod::write_memory(pounce, jmp.data(), jmp.size());
}

template <typename T, typename U>
void setup_call(T *pounce, U *splat)
{
    auto jmp = zmod::parse_hex("E8 ?? ?? ?? ??", rel(pounce + 5, splat));
    zmod::write_memory(pounce, jmp.data(), jmp.size());
}

void wait_loop(LARGE_INTEGER *lpPerformanceCount, uint64_t vsync_30)
{
    auto dt_target = globals.dt_target;
    auto dt_sleep = globals.dt_sleep;
    auto lfcm = globals.late_frame_compensation_multiplier;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    auto dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
    if (dt < dt_target * lfcm)
    {
        while (dt < dt_sleep)
        {
            Sleep(1);
            QueryPerformanceCounter(&now);
            dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
        }
        while (dt < dt_target)
        {
            QueryPerformanceCounter(&now);
            dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
        }
        lpPerformanceCount->QuadPart = lpPerformanceCount->QuadPart + (LONGLONG)(dt_target * globals.frequency); // Prevent drift
    }
    else
    {
        lpPerformanceCount->QuadPart = now.QuadPart; // Prevent the game from speeding up if dt >= dt_target * lfcm
    }
}

void setup_wait_loop_hook()
{
    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto addr = zmod::find_pattern(wo4u, 0xFFFFFF, "48 8B 8F 08 18 00 00") - 5;
    setup_call(addr, wait_loop);
}

void setup_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C9 48 8B C8");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 0D 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    setup_jmp(jmp_site, code);
}

void setup_camera_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 F3 0F 11 46 10");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    setup_jmp(jmp_site, code);
}

void setup_musou_camera_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 F3 0F 11 84 8B 78 01 00 00");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    setup_jmp(jmp_site, code);
}

void setup_physics_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A F0 F3 0F 59 35 ?? ?? ?? ?? 48 83 7F 48 00");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 35 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    setup_jmp(jmp_site, code);
}

// Particle lifetime is based on frame rate. This hook only adjusts particle and effect speed.
void setup_effects_speed_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 48 0F 2A C0 8B 01");
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    setup_jmp(jmp_site, code);
}

void setup_rim_lighting_hook()
{
    static const uint8_t code[64] = {};
    zmod::unprotect(code, sizeof(code));

    auto wo4u = zmod::get_base_address(L"WO4U.dll");
    auto jmp_site = zmod::find_pattern(wo4u, 0xFFFFFF, "F3 0F 58 83 D8 34 00 00") - 13;
    auto speed = (float)(60.0 / globals.target_fps);
    auto patch = zmod::parse_hex("F3 0F 10 05 05 00 00 00 E9 ?? ?? ?? ?? ?? ?? ?? ??", rel(code + 13, jmp_site + 5), speed);
    zmod::write_memory_unsafe(code, patch.data(), patch.size());

    setup_jmp(jmp_site, code);
}

void module_main(HINSTANCE hinstDLL)
{
    zmod::ini ini = {
        {{L"zmod_wo4u_performance", L"global_speed_multiplier"}, L"1.0"},
        {{L"zmod_wo4u_performance", L"late_frame_compensation_multiplier"}, L"1.0"},
        {{L"zmod_wo4u_performance", L"target_fps"}, L"60.0"},
    };

    auto ini_path = zmod::get_module_path(hinstDLL).replace_filename(L"zmod_wo4u_performance.ini");
    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    zmod::set_timer_resolution();
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    globals.frequency = (double)frequency.QuadPart;

    auto target_fps = std::wcstof(ini[{L"zmod_wo4u_performance", L"target_fps"}].c_str(), nullptr);
    auto dt_target = 1.0 / (target_fps * std::wcstod(ini[{L"zmod_wo4u_performance", L"global_speed_multiplier"}].c_str(), nullptr));

    globals.target_fps = target_fps;
    globals.dt_target = dt_target;
    globals.dt_sleep = dt_target - 0.0016;
    globals.late_frame_compensation_multiplier = std::max(1.0, std::wcstod(ini[{L"zmod_wo4u_performance", L"late_frame_compensation_multiplier"}].c_str(), nullptr));

    setup_wait_loop_hook();
    setup_speed_hook();
    setup_camera_speed_hook();
    setup_musou_camera_speed_hook();
    setup_physics_speed_hook();
    setup_effects_speed_hook();
    setup_rim_lighting_hook();
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        if (zmod::get_module_path(NULL).filename().wstring().find(L"WO4.exe") == std::wstring::npos)
        {
            return FALSE;
        }
        module_main(hinstDll);
        break;
    }
    return TRUE;
}