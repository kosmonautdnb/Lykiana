call c:\opwat20\setvars.bat
set INCLUDE=%INCLUDE%;SRC\STL;SRC\IMGUI;
_build\text.exe
@echo off
@echo Starting.. (needs RamDisk at R:)
@echo build errors > R:\LOGS.LOG
_BUILD\FLTMECHK.exe !MAK R:\main.exe
if not ERRORLEVEL 255 call _BUILD\MAKEMK.BAT
SET DJGPPCOMPILE=0
rem @echo system dos4g > R:\LINK.LNK
@echo system pmodew > R:\LINK.LNK
call !clean.bat
call wmake.exe /f R:\makefile /e /s /k
copy r:\main.exe nora1_ow.exe
SET DJGPPCOMPILE=1
@echo -oR:\main.exe > R:\LINK2.LNK
call !clean.bat
call wmake.exe /f R:\makefile /e /s /k
copy r:\main.exe nora1_dj.exe
SET DJGPPCOMPILE=0
