:: This file is originally from obs-studio and was modified on 2024-03-01
@echo off
@cd /d "%~dp0"
goto checkAdmin

:checkAdmin
	net session >nul 2>&1
	if %errorLevel% == 0 (
		echo.
	) else (
		echo Administrative rights are required, please re-run this script as Administrator.
		goto end
	)

:uninstallDLLs
	if exist "%~dp0\win-dshow\open-virtualcam-module32.dll" (
		regsvr32.exe /u /s "%~dp0\win-dshow\open-virtualcam-module32.dll"
	) else (
		regsvr32.exe /u /s open-virtualcam-module32.dll
	)
	if exist "%~dp0\win-dshow\open-virtualcam-module64.dll" (
		regsvr32.exe /u /s "%~dp0\win-dshow\open-virtualcam-module64.dll"
	) else (
		regsvr32.exe /u /s open-virtualcam-module64.dll
	)

:endSuccess
	echo Virtual Cam uninstalled!
	echo.

:end
	echo.
	
