for %%A in (".\x64_artifacts" ".\x64_release") do if not exist %%A mkdir %%A
cl.exe ^
/EHsc ^
/std:c++latest ^
/Ox ^
/LD ^
/I ../include ^
/Fe./x64_artifacts/zmod_wo4u ^
/Fo./x64_artifacts/zmod_wo4u ^
../wo4u.cpp ^
/link ../lib.X64/detours.lib winmm.lib user32.lib dwmapi.lib
move /Y .\x64_artifacts\*.dll .\x64_release
move /Y .\x64_artifacts\*.exe .\x64_release