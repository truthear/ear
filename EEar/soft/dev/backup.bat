@echo off
rar a -r -m1 ear _other client common server server_http server_out
if %ERRORLEVEL% NEQ 0 pause
move /Y ear.rar ..\..\..\..\dev\
if %ERRORLEVEL% NEQ 0 pause

echo _
echo _
echo _
echo        лллллллл  лл   лл   лл лл лл
echo        лллллллл  лл  ллл   лл лл лл
echo        лл    лл  лл ллл    лл лл лл
echo        лл    лл  ллллл     лл лл лл
echo        лл    лл  ллллл     лл лл лл
echo        лл    лл  лл ллл    лл лл лл
echo        лллллллл  лл  ллл           
echo        лллллллл  лл   лл   лл лл лл
echo _
echo _
echo _
pause
