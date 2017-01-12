@echo off
cl -Fetest -EHsc ..\..\client\aes.c ..\..\client\crc32.c ..\..\server\ourtime.cpp *.cpp ws2_32.lib
del *.obj
