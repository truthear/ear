@echo off
cl -Fetest_sender -DSENDER -EHsc *.cpp shlwapi.lib
del *.obj
cl -Fetest_receiver -EHsc *.cpp shlwapi.lib
del *.obj
