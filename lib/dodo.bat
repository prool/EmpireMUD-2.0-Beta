@echo off
del stop.txt
:label1
evolve.exe
if exist "stop.txt" (
del stop.txt
exit
)
timeout /t 300
goto label1
