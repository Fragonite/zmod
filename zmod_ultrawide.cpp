#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <filesystem>
#include <map>
#include "detours.h"
#include "io_helper.cpp"

#define IDirect3DDevice9 void
#define D3DPRESENT_PARAMETERS void

struct
{
    uint32_t width;
    uint32_t height;
    double aspectRatio;
    bool windowed;
    ini_map iniMap;
} static globals;

LSTATUS(WINAPI *_RegQueryValueExA)
(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) = RegQueryValueExA;

LSTATUS(WINAPI *_RegSetValueExA)
(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData) = RegSetValueExA;

HRESULT(APIENTRY *_Reset)
(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);

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
    ((uint32_t *)pPresentationParameters)[8] = globals.windowed;
    return _Reset(pDevice, pPresentationParameters);
}

void WriteMemory(void *address, void *data, size_t size)
{
    DWORD oldProtect;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, data, size);
    VirtualProtect(address, size, oldProtect, &oldProtect);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (DisableThreadLibraryCalls(hinstDLL))
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
        WriteMemory((void *)(base + 0x22DD1F), &aspectRatioPatch, sizeof(aspectRatioPatch)); // Aspect Ratio

        if (!(globals.windowed))
        {
            uint8_t hideCursorPatch = 0x01;
            WriteMemory((void *)(base + 0x5CA253), &hideCursorPatch, sizeof(hideCursorPatch)); // Hide Cursor When Fullscreen
        }

        _Reset = (HRESULT(APIENTRY *)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *))((uint32_t)GetModuleHandleW(L"d3d9.dll") + 0xE45A0);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)_RegQueryValueExA, HookedRegQueryValueExA);
        DetourAttach(&(PVOID &)_RegSetValueExA, HookedRegSetValueExA);
        DetourAttach(&(PVOID &)_Reset, HookedReset);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}