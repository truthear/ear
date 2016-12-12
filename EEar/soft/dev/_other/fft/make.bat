@echo off

cl -O2t -EHsc gui_analyzer.cpp user32.lib gdi32.lib
if %ERRORLEVEL% NEQ 0 pause
del *.obj

cl -Fefd_test -O2t -EHsc fd_test.cpp fdetector.cpp
if %ERRORLEVEL% NEQ 0 pause
del *.obj
