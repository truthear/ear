@echo off
if exist ewarm\out rmdir /S /Q ewarm\out
iarbuild ewarm\Project.ewp -build STM32F40_41xxx
if %ERRORLEVEL% NEQ 0 pause
copy /Y ewarm\out\exe\*.bin .\

