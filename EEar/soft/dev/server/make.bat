@echo off

set cl=

rc /nologo resource.rc

cl /Feearsvc /nologo /MT /EHsc /O2t /DNDEBUG /DUSE_AES_DEC *.cpp ..\client\aes.c ..\client\crc32.c -link /MACHINE:X86 /TSAWARE /STACK:131072 kernel32.lib user32.lib advapi32.lib shlwapi.lib shell32.lib ws2_32.lib crypt32.lib sqlite\x86\sqlite.lib resource.res
if %ERRORLEVEL% NEQ 0 pause

if exist *.res del *.res
if exist *.obj del *.obj

