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

:uninstallVCam
	CALL win-dshow\virtualcam-uninstall.bat
	@cd /d "%~dp0"

:uninstallInStartup
	del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\vcam.exe"
	if %errorLevel% == 0 (
		echo.
	) else (
		echo failed removing %APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\vcam.exe
		echo.
		goto end
	)

:endSuccess
	echo Uninstalled Open Virtual Camera!
	echo.
	taskkill /im vcam.exe

:end
	pause

