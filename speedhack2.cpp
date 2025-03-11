#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <map>
#include <filesystem>
#include <thread>
#include "detours.h"
#include "zmod_common.cpp"

typedef BOOL(WINAPI QueryPerformanceCounter_t)(LARGE_INTEGER *lpPerformanceCount);
QueryPerformanceCounter_t *QueryPerformanceCounter_orig = QueryPerformanceCounter;

struct
{
    double speedhack;
    double speedhack0;
    long long snapshot;
    long long future_offset;
    long long accumulated;
    CRITICAL_SECTION cs;
} globals;

BOOL WINAPI QueryPerformanceCounter_hook(LARGE_INTEGER *lpPerformanceCount)
{
    // EnterCriticalSection(&globals.cs);
    QueryPerformanceCounter_orig(lpPerformanceCount);
    auto accumulated = lpPerformanceCount->QuadPart - globals.snapshot;
    lpPerformanceCount->QuadPart = (globals.snapshot + accumulated * globals.speedhack) + globals.future_offset;
    // DeleteCriticalSection(&globals.cs);
    return true;
}

void initialise_speedhack(zmod::ini &ini)
{
    // EnterCriticalSection(&globals.cs);
    globals.speedhack = ini.get_double({L"zmod_speedhack", L"global_speed_multiplier"});
    globals.speedhack0 = 1.0;
    LARGE_INTEGER now;
    QueryPerformanceCounter_orig(&now);
    globals.snapshot = now.QuadPart;
    globals.future_offset = 0;
    // LeaveCriticalSection(&globals.cs);
}

double set_speedhack(double speed)
{
    // EnterCriticalSection(&globals.cs);
    auto old_speed = globals.speedhack;
    LARGE_INTEGER now_orig;
    QueryPerformanceCounter_orig(&now_orig);
    LARGE_INTEGER now_hook;
    QueryPerformanceCounter_hook(&now_hook);
    globals.snapshot = now_orig.QuadPart;
    globals.future_offset = now_hook.QuadPart - now_orig.QuadPart;
    globals.speedhack = speed;
    // LeaveCriticalSection(&globals.cs);
    return old_speed;
}

void speedhack_toggle_listener()
{
    auto disabled = false;
    auto speed = 1.0;
    while (true)
    {
        if (GetAsyncKeyState(VK_F24) & 0x8000)
        {
            if (disabled)
            {
                disabled = false;
                set_speedhack(speed);
            }
            else
            {
                disabled = true;
                speed = set_speedhack(1.0);
            }
            Sleep(500);
        }
        else
        {
            Sleep(50);
        }
    }
}

void module_main(HINSTANCE hinstDLL)
{
    auto ini = zmod::ini(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_speedhack.ini"));
    ini.set_many({
        {{L"zmod_speedhack", L"global_speed_multiplier"}, L"1.0"},
        {{L"zmod_speedhack", L"process_priority_class"}, L"0x00000080"},
    });
    if (ini.exists())
    {
        ini.load();
    }
    else
    {
        ini.save();
    }

    // InitializeCriticalSection(&globals.cs);

    initialise_speedhack(ini);

    zmod::set_timer_resolution();

    auto process_priority_class = ini.get_uint({L"zmod_speedhack", L"process_priority_class"}, 0);
    if (process_priority_class)
    {
        if (!(SetPriorityClass(GetCurrentProcess(), process_priority_class)))
            ;
    }

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID &)QueryPerformanceCounter_orig, QueryPerformanceCounter_hook);
    DetourTransactionCommit();

    std::thread(speedhack_toggle_listener).detach();

    // DeleteCriticalSection(&globals.cs);
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        module_main(hinstDll);
        break;
    }
    return TRUE;
}