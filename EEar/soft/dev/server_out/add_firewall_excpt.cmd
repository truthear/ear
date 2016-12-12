@echo off
netsh firewall add allowedprogram "%~dp0earsvc.exe" EarSvc ENABLE ALL
