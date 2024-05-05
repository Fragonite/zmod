for %%A in (".\x86_artifacts" ".\x86_release") do if not exist %%A mkdir %%A
cl.exe ^
/EHsc ^
/std:c++latest ^
/Ox ^
/LD ^
/I ../include ^
/Fe./x86_artifacts/dinput8 ^
/Fo./x86_artifacts/dinput8 ^
../zmod_dll.cpp ^
/link ../lib.X86/detours.lib user32.lib winmm.lib ^
/DEF:../zmod_32.def
move /Y .\x86_artifacts\*.dll .\x86_release
move /Y .\x86_artifacts\*.exe .\x86_release