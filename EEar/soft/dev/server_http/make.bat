@echo off
if exist mod_ear.dll del mod_ear.dll
cl -nologo -Femod_ear -LD -EHsc -O2t *.cpp ..\server\ourtime.cpp ..\server\tools.cpp ..\server\sqlite\*.cpp ..\server\sqlite\*.c -link /DEF:exp.def shlwapi.lib ws2_32.lib crypt32.lib
if %ERRORLEVEL% NEQ 0 pause
del *.obj
del *.lib
del *.exp
move /Y mod_ear.dll ..\server_out\
if %ERRORLEVEL% NEQ 0 pause



