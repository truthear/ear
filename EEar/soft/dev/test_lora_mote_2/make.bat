@echo off
cl -Fetest_receiver_125_SF7 -DMODE125SF7 -EHsc *.cpp shlwapi.lib ole32.lib user32.lib locationapi.lib
cl -Fetest_receiver_125_SF10 -DMODE125SF10 -EHsc *.cpp shlwapi.lib ole32.lib user32.lib locationapi.lib
cl -Fetest_receiver_125_SF12 -DMODE125SF12 -EHsc *.cpp shlwapi.lib ole32.lib user32.lib locationapi.lib
cl -Fetest_receiver_500_SF7 -DMODE500SF7 -EHsc *.cpp shlwapi.lib ole32.lib user32.lib locationapi.lib
cl -Fetest_receiver_500_SF10 -DMODE500SF10 -EHsc *.cpp shlwapi.lib ole32.lib user32.lib locationapi.lib
cl -Fetest_receiver_500_SF12 -DMODE500SF12 -EHsc *.cpp shlwapi.lib ole32.lib user32.lib locationapi.lib
del *.obj
