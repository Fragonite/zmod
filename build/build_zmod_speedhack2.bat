for %%A in (".\x64_artifacts" ".\x64_release") do if not exist %%A mkdir %%A
cl.exe ^
/EHsc ^
/std:c++latest ^
/O2 ^
/LD ^
/I ../include ^
/Fe./x64_artifacts/zmod_speedhack2 ^
/Fo./x64_artifacts/zmod_speedhack2 ^
../speedhack2.cpp ^
/link ../lib.X64/detours.lib winmm.lib user32.lib
move /Y .\x64_artifacts\*.dll .\x64_release
move /Y .\x64_artifacts\*.exe .\x64_release