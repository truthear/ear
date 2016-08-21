@echo off
if exist mod_ear.dll del mod_ear.dll
rc /nologo resource.rc
if %ERRORLEVEL% NEQ 0 pause
cl -nologo -Femod_ear -LD -EHsc -O2t *.cpp ..\server\ourtime.cpp ..\server\tools.cpp ..\server\config.cpp ..\server\sqlite\*.cpp ..\server\sqlite\*.c -link /DEF:exp.def /MACHINE:X86 shlwapi.lib ws2_32.lib crypt32.lib resource.res
if %ERRORLEVEL% NEQ 0 pause
del *.obj
del *.lib
del *.exp
del *.res
move /Y mod_ear.dll ..\server_out\
if %ERRORLEVEL% NEQ 0 pause



