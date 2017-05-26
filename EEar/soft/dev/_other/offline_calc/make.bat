@echo off
cl -EHsc -O2t -Feoffline_calc *.cpp
del *.obj
