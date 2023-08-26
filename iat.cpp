#include <windows.h>
#include <iostream>
#include <Xinput.h>

// #pragma comment(lib, "XInput")

int main()
{
    XINPUT_STATE state{};
    auto dwResult = XInputGetState(0, &state);

    // cout the system32 path
    char system32_path[MAX_PATH];
    GetSystemDirectoryA(system32_path, MAX_PATH);
    std::cout << "System32 Path: " << system32_path << std::endl;

    HMODULE hModule = GetModuleHandle(NULL);

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)hModule;
    IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS *)((BYTE *)hModule + pDosHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY importDirectory = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    IMAGE_IMPORT_DESCRIPTOR *pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR *)((BYTE *)hModule + importDirectory.VirtualAddress);

    for (; pImportDescriptor->Name; ++pImportDescriptor)
    {
        char *dllName = (char *)((BYTE *)hModule + pImportDescriptor->Name);
        std::cout << "DLL Name: " << dllName << std::endl;

        IMAGE_THUNK_DATA *pOriginalThunk = (IMAGE_THUNK_DATA *)((BYTE *)hModule + pImportDescriptor->OriginalFirstThunk);
        IMAGE_THUNK_DATA *pThunk = (IMAGE_THUNK_DATA *)((BYTE *)hModule + pImportDescriptor->FirstThunk);

        for (; pOriginalThunk->u1.AddressOfData; ++pOriginalThunk, ++pThunk)
        {
            if (pOriginalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
            {
                // Imported by ordinal
                WORD ordinal = IMAGE_ORDINAL(pOriginalThunk->u1.Ordinal);
                std::cout << "Imported by Ordinal: " << ordinal << std::endl;
                DWORD oldProtect;
                if (!(VirtualProtect((LPVOID)pOriginalThunk, sizeof(IMAGE_THUNK_DATA), PAGE_EXECUTE_READWRITE, &oldProtect)))
                {
                    std::cout << "Failed to change memory protection" << std::endl;
                }
                else
                {
                    std::cout << "Memory protection changed" << std::endl;
                }
            }
            else
            {
                // Imported by name
                IMAGE_IMPORT_BY_NAME *pImportByName = (IMAGE_IMPORT_BY_NAME *)((BYTE *)hModule + pOriginalThunk->u1.AddressOfData);
                if (pImportByName)
                {
                    char *funcName = (char *)pImportByName->Name;

                    // if (funcName)
                    // {
                    //     for (int i = 0; i < 10; ++i)
                    //     {
                    //         __try
                    //         {
                    //             DWORD oldProtect;
                    //             if (VirtualProtect((LPVOID)pImportByName, sizeof(IMAGE_IMPORT_BY_NAME), PAGE_EXECUTE_READWRITE, &oldProtect))
                    //                 ;
                    //             std::cout << "Byte[" << i << "]: " << static_cast<int>(funcName[i]) << std::endl;
                    //         }
                    //         __except (EXCEPTION_EXECUTE_HANDLER)
                    //         {
                    //             std::cout << "Exception reading byte " << i << std::endl;
                    //             break;
                    //         }
                    //     }

                    // for (int i = 0; i < 10; ++i)
                    // {
                    //     std::cout << "Byte[" << i << "]: " << static_cast<int>(funcName[i]) << std::endl;
                    // }
                    std::cout << "Function Name: " << funcName << std::endl;
                    // }
                }
            }

            FARPROC *funcAddrPtr = (FARPROC *)&(pThunk->u1.Function);
            std::cout << "Function Address: " << funcAddrPtr << " -> " << *funcAddrPtr << std::endl;
        }
    }
    MessageBoxA(NULL, "test", "hello", MB_OK);

    return 0;
}