cl.exe /EHsc /std:c++latest /Ox /LD /I include zmod_dll.cpp /link lib.X86/detours.lib user32.lib winmm.lib /def:zmod_32.def /out:dinput8.dll
