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

:checkDLL
	echo Checking for 32-bit Virtual Cam registration...
	reg query "HKLM\SOFTWARE\Classes\WOW6432Node\CLSID\{689685A0-B481-11EE-9EC1-0800200C9A66}" >nul 2>&1
	if %errorLevel% == 0 (
		echo 32-bit Virtual Cam found, skipping install...
		echo.
	) else (
		echo 32-bit Virtual Cam not found, installing...
		goto install32DLL
	)

:CheckDLLContinue
	echo Checking for 64-bit Virtual Cam registration...
	reg query "HKLM\SOFTWARE\Classes\CLSID\{689685A0-B481-11EE-9EC1-0800200C9A66}" >nul 2>&1
	if %errorLevel% == 0 (
		echo 64-bit Virtual Cam found, skipping install...
		echo.
	) else (
		echo 64-bit Virtual Cam not found, installing...
		goto install64DLL
	)
	goto endSuccess

:install32DLL
	echo Installing 32-bit Virtual Cam...
	if exist "%~dp0\win-dshow\open-virtualcam-module32.dll" (
		regsvr32.exe /i /s "%~dp0\win-dshow\open-virtualcam-module32.dll"
	) else (
		regsvr32.exe /i /s open-virtualcam-module32.dll
	)
	reg query "HKLM\SOFTWARE\Classes\WOW6432Node\CLSID\{689685A0-B481-11EE-9EC1-0800200C9A66}" >nul 2>&1
	if %errorLevel% == 0 (
		echo 32-bit Virtual Cam successfully installed
		echo.
	) else (
		echo 32-bit Virtual Cam installation failed
		echo.
	)
	goto checkDLLContinue

:install64DLL
	echo Installing 64-bit Virtual Cam...
	if exist "%~dp0\win-dshow\open-virtualcam-module64.dll" (
		regsvr32.exe /i /s "%~dp0\win-dshow\open-virtualcam-module64.dll"
	) else (
		regsvr32.exe /i /s open-virtualcam-module64.dll
	)
	reg query "HKLM\SOFTWARE\Classes\CLSID\{689685A0-B481-11EE-9EC1-0800200C9A66}" >nul 2>&1
	if %errorLevel% == 0 (
		echo 64-bit Virtual Cam successfully installed
		echo.
		goto endSuccess
	) else (
		echo 64-bit Virtual Cam installation failed
		echo.
		goto end
	)

:endSuccess
	echo Virtual Cam installed!
	echo.

:end
	echo.
