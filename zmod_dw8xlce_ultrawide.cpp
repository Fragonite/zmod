#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <filesystem>
#include <map>
#include "d3d9.h"
#include "zmod_dw8xlce_ultrawide.hpp"
#include "detours.h"
#include "io_helper.cpp"

struct
{
    uint32_t width;
    uint32_t height;
    double aspectRatio;
    bool windowed;
    ini_map iniMap;
} static globals;

Direct3DCreate9_t *_Direct3DCreate9 = (Direct3DCreate9_t *)GetProcAddress(GetModuleHandleA("d3d9.dll"), "Direct3DCreate9");
CreateDevice_t *_CreateDevice = nullptr;
Reset_t *_Reset = nullptr;
RegQueryValueExA_t *_RegQueryValueExA = RegQueryValueExA;
RegSetValueExA_t *_RegSetValueExA = RegSetValueExA;
GetSystemMetrics_t *_GetSystemMetrics = GetSystemMetrics;
function_22DC70_t *_AspectRatio = nullptr;
// function_1158A0_t *_AspectRatio = nullptr;

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

void WriteMemory(void *address, void *data, size_t size)
{
    DWORD oldProtect;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, data, size);
    VirtualProtect(address, size, oldProtect, &oldProtect);
}

int WINAPI HookedGetSystemMetrics(int nIndex)
{
    if (nIndex == 0)
    {
        return globals.width;
    }
    else if (nIndex == 1)
    {
        return 2160; // This hook is a fix for getting 2560x1080 to run in fullscreen without running a higher resolution first
    }
    return _GetSystemMetrics(nIndex);
}

void __stdcall HookedAspectRatio(uint32_t a0, float *a1)
{
    if (a1 != nullptr)
    {
        if (a1[6] == 360.0)
        {
            globals.aspectRatio = (double)(globals.width * 2) / (double)globals.height;
        }
        else
        {
            globals.aspectRatio = (double)globals.width / (double)globals.height;
        }
    }
    _AspectRatio(a0, a1);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;

        std::wstring moduleName;
        moduleName.reserve(MAX_PATH);
        if (!(GetModuleFileNameW(hinstDLL, moduleName.data(), moduleName.capacity())))
            ;
        auto modulePath = std::filesystem::path(moduleName.c_str());
        auto iniPath = modulePath.replace_extension(L".ini");
        std::wstring moduleKey = modulePath.stem().c_str();

        globals.iniMap = {
            {{moduleKey, L"width"}, L"1280"},
            {{moduleKey, L"height"}, L"720"},
            {{moduleKey, L"fullscreen"}, L"0"},
        };

        if (!(file_exists(iniPath)))
        {
            create_ini_file(iniPath, globals.iniMap);
        }
        read_ini_file(iniPath, globals.iniMap);

        globals.width = std::wcstoul(globals.iniMap[{moduleKey, L"width"}].c_str(), nullptr, 10);
        globals.height = std::wcstoul(globals.iniMap[{moduleKey, L"height"}].c_str(), nullptr, 10);
        globals.aspectRatio = (double)globals.width / (double)globals.height;
        globals.windowed = globals.iniMap[{moduleKey, L"fullscreen"}] != L"1";

        auto base = (uint32_t)GetModuleHandleW(nullptr);
        WriteMemory((void *)(base + 0x3753), &globals.width, sizeof(globals.width));     // 1280 (Executable) Width
        WriteMemory((void *)(base + 0x5CC104), &globals.width, sizeof(globals.width));   // 1280 (Executable) Window Width
        WriteMemory((void *)(base + 0x600184), &globals.width, sizeof(globals.width));   // 1920 (Memory) Necessary
        WriteMemory((void *)(base + 0x5BFC3D), &globals.height, sizeof(globals.height)); // 720 (Executable) Window Height
        WriteMemory((void *)(base + 0x585E8D), &globals.width, sizeof(globals.width));   // 1280 (Executable) HUD Fix
        WriteMemory((void *)(base + 0x5C1D8C), &globals.width, sizeof(globals.width));   // 1280 (Executable) Rage HUD Fix

        uint8_t aspectRatioPatch[6] = {0xDD, 0x05};
        auto aspectRatioPointer = &globals.aspectRatio;
        memcpy(&aspectRatioPatch[2], &aspectRatioPointer, 4);
        float cutsceneAspectRatio = (float)globals.width / (float)globals.height;
        float multiplayerAspectRatio = (float)(globals.width * 2) / (float)globals.height;

        WriteMemory((void *)(base + 0x22DD1F), &aspectRatioPatch, sizeof(aspectRatioPatch));             // Aspect Ratio
        WriteMemory((void *)(base + 0x61DF10), &cutsceneAspectRatio, sizeof(cutsceneAspectRatio));       // Fixed Cutscene Aspect Ratio
        WriteMemory((void *)(base + 0x61DF18), &multiplayerAspectRatio, sizeof(multiplayerAspectRatio)); // Fixed Multiplayer Cutscene Aspect Ratio

        _AspectRatio = (function_22DC70_t *)(base + 0x22DC70);

        if (!(globals.windowed))
        {
            uint8_t hideCursorPatch = 0x01;
            WriteMemory((void *)(base + 0x5CA253), &hideCursorPatch, sizeof(hideCursorPatch)); // Hide Cursor When Fullscreen
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
        break;
    }
    return TRUE;
}