@echo off
cl -O2t -EHsc *.cpp user32.lib gdi32.lib
if %ERRORLEVEL% NEQ 0 pause
del *.obj
