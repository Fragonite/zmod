for %%A in (".\x64_artifacts" ".\x64_release") do if not exist %%A mkdir %%A
cl.exe ^
/EHsc ^
/std:c++latest ^
/Ox ^
/LD ^
/I ../include ^
/Fe./x64_artifacts/dinput8 ^
/Fo./x64_artifacts/dinput8 ^
../zmod_dll.cpp ^
/link ../lib.X64/detours.lib user32.lib winmm.lib
move /Y .\x64_artifacts\*.dll .\x64_release
move /Y .\x64_artifacts\*.exe .\x64_release