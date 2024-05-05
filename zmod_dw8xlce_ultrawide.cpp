// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>
// #include <string>
// #include <filesystem>
// #include <map>
// #include <vector>
// #include <sstream>
// #include "d3d9.h"
// #include "detours.h"
// #include "zmod_common.cpp"

namespace ultrawide
{
    struct
    {
        uint32_t width;
        uint32_t height;
        double aspect_ratio;
        bool windowed;
    } globals;

    typedef IDirect3D9 *(WINAPI Direct3DCreate9_t)(UINT SDKVersion);
    Direct3DCreate9_t HookedDirect3DCreate9;

    typedef HRESULT(APIENTRY CreateDevice_t)(IDirect3D9 *pD3D9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface);
    CreateDevice_t HookedCreateDevice;

    typedef HRESULT(APIENTRY Reset_t)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
    Reset_t HookedReset;

    typedef LSTATUS(WINAPI RegQueryValueExA_t)(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
    RegQueryValueExA_t HookedRegQueryValueExA;

    typedef LSTATUS(WINAPI RegSetValueExA_t)(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData);
    RegSetValueExA_t HookedRegSetValueExA;

    typedef int(WINAPI GetSystemMetrics_t)(int nIndex);
    GetSystemMetrics_t HookedGetSystemMetrics;

    typedef void(__stdcall function_22DC70_t)(uint32_t a0, float *a1);
    function_22DC70_t HookedAspectRatio;

    Direct3DCreate9_t *_Direct3DCreate9 = (Direct3DCreate9_t *)GetProcAddress(GetModuleHandleA("d3d9.dll"), "Direct3DCreate9");
    CreateDevice_t *_CreateDevice = nullptr;
    Reset_t *_Reset = nullptr;
    RegQueryValueExA_t *_RegQueryValueExA = RegQueryValueExA;
    RegSetValueExA_t *_RegSetValueExA = RegSetValueExA;
    GetSystemMetrics_t *_GetSystemMetrics = GetSystemMetrics;
    function_22DC70_t *_AspectRatio = nullptr;

    IDirect3D9 *WINAPI HookedDirect3DCreate9(UINT SDKVersion)
    {
        auto rv = _Direct3DCreate9(SDKVersion);
        if (rv == nullptr)
            ;

        auto vtable = *(uint32_t **)rv;
        _CreateDevice = (CreateDevice_t *)(vtable[16]);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID &)_Direct3DCreate9, HookedDirect3DCreate9);
        DetourAttach(&(PVOID &)_CreateDevice, HookedCreateDevice);
        DetourTransactionCommit();

        return rv;
    }

    HRESULT APIENTRY HookedCreateDevice(IDirect3D9 *pD3D9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface)
    {
        auto rv = _CreateDevice(pD3D9, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
        if (rv != D3D_OK)
            ;

        auto vtable = *(uint32_t **)(*ppReturnedDeviceInterface);
        _Reset = (Reset_t *)vtable[16];

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID &)_CreateDevice, HookedCreateDevice);
        DetourAttach(&(PVOID &)_Reset, HookedReset);
        DetourTransactionCommit();

        return rv;
    }

    LSTATUS WINAPI HookedRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    {
        auto rv = _RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
        if (rv == ERROR_SUCCESS)
        {
            if (std::string(lpValueName) == "Resolution")
            {
                *(DWORD *)lpData = 3;
            }
            else if (std::string(lpValueName) == "WindowMode")
            {
                *(DWORD *)lpData = 1;
            }
        }
        return rv;
    }

    LSTATUS WINAPI HookedRegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
    {
        if (std::string(lpValueName) == "Resolution")
        {
            return ERROR_SUCCESS;
        }
        else if (std::string(lpValueName) == "WindowMode")
        {
            return ERROR_SUCCESS;
        }
        return _RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
    }

    HRESULT APIENTRY HookedReset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
    {
        pPresentationParameters->Windowed = globals.windowed;
        return _Reset(pDevice, pPresentationParameters);
    }

    int WINAPI HookedGetSystemMetrics(int nIndex)
    {
        if (nIndex == 0)
        {
            return globals.width;
        }
        else if (nIndex == 1)
        {
            // This hook is a fix for getting 2560x1080 to run in fullscreen without running a higher resolution first.
            return std::max(globals.height, (uint32_t)2160);
        }
        return _GetSystemMetrics(nIndex);
    }

    void __stdcall HookedAspectRatio(uint32_t a0, float *a1)
    {
        if (a1 != nullptr)
        {
            if (a1[6] == 360.0)
            {
                globals.aspect_ratio = (double)(globals.width * 2) / (double)globals.height;
            }
            else
            {
                globals.aspect_ratio = (double)globals.width / (double)globals.height;
            }
        }
        _AspectRatio(a0, a1);
    }

    void set_ini_defaults(zmod::ini &ini)
    {
        ini.set_many({
            {{L"ultrawide", L"width"}, L"auto"},
            {{L"ultrawide", L"height"}, L"auto"},
            {{L"ultrawide", L"fullscreen"}, L"1"},
        });
    }

    void module_main(zmod::ini &ini)
    {
        auto width = ini.get_wstring({L"ultrawide", L"width"});
        if (zmod::to_lower(width) == L"auto")
        {
            globals.width = GetSystemMetrics(SM_CXSCREEN);
        }
        else
        {
            globals.width = ini.get_uint({L"ultrawide", L"width"});
        }

        auto height = ini.get_wstring({L"ultrawide", L"height"});
        if (zmod::to_lower(height) == L"auto")
        {
            globals.height = GetSystemMetrics(SM_CYSCREEN);
        }
        else
        {
            globals.height = ini.get_uint({L"ultrawide", L"height"});
        }
        globals.aspect_ratio = (double)globals.width / (double)globals.height;
        globals.windowed = (ini.get_bool({L"ultrawide", L"fullscreen"})) ? false : true;

        struct patch
        {
            void *address;
            void *data;
            size_t size;
        };

        auto ar = zmod::parse_hex("DD 05 00 00 00 00");
        {
            auto p = &globals.aspect_ratio;
            std::memcpy(ar.data() + 2, &p, 4);
        }
        float ar_sp_cutscene = globals.aspect_ratio;
        float ar_mp_cutscene = globals.aspect_ratio * 2;

        std::vector<patch> patches = {
            {(void *)(zmod::find_pattern("C3 B8 00 05") + 2), &globals.width, 4},           // 1280 (Executable) Width
            {(void *)(zmod::find_pattern("B8 00 04 00 00 6A 40") - 20), &globals.width, 4}, // 1280 (Executable) Window Width
            {(void *)(zmod::find_pattern("B8 D0 02 00 00 5B") + 1), &globals.height, 4},    // 720 (Executable) Window Height
            {(void *)(zmod::find_pattern("C7 44 24 0C 00 05") + 4), &globals.width, 4},     // 1280 (Executable) HUD Fix
            {(void *)(zmod::find_pattern("10 00 05 00 00 EB 1C") + 1), &globals.width, 4},  // 1280 (Executable) Rage HUD Fix
            {(void *)(zmod::find_pattern("80 07 00 00 E0 01 58 02")), &globals.width, 2},   // 1920 (Memory) Necessary
            {(void *)(zmod::find_pattern("D9 43 14 D8 73 18")), ar.data(), ar.size()},      // Aspect Ratio
            {(void *)(zmod::find_pattern("40 39 8E E3") + 1), &ar_sp_cutscene, 4},          // Single Player Cutscene Aspect Ratio
            {(void *)(zmod::find_pattern("40 39 8E E3") + 9), &ar_mp_cutscene, 4},          // Multiplayer Cutscene Aspect Ratio
        };

        for (auto &p : patches)
        {
            zmod::write_memory(p.address, p.data, p.size);
        }

        _AspectRatio = (function_22DC70_t *)(zmod::find_pattern("33 C4 89 44 24 44 D9 05") - 8);

        if (!(globals.windowed))
        {
            uint8_t hide_cursor_patch = 0x01;
            zmod::write_memory((void *)(zmod::find_pattern("10 00 E8 ?? ?? ?? ?? 80 B8") + 13), &hide_cursor_patch, 1);
        }

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)_Direct3DCreate9, HookedDirect3DCreate9);
        DetourAttach(&(PVOID &)_RegQueryValueExA, HookedRegQueryValueExA);
        DetourAttach(&(PVOID &)_RegSetValueExA, HookedRegSetValueExA);
        if (!(globals.windowed))
        {
            DetourAttach(&(PVOID &)_GetSystemMetrics, HookedGetSystemMetrics);
        }
        DetourAttach(&(PVOID &)_AspectRatio, HookedAspectRatio);
        DetourTransactionCommit();
    }
}