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