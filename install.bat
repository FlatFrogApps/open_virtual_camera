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

:installVCam
	CALL win-dshow\virtualcam-install.bat
	@cd /d "%~dp0"

	SET /P AREYOUSURE=Would you like Open Virtual Camera to run on startup (Y/[N])?
	IF /I "%AREYOUSURE%" NEQ "Y" (
		echo Will not add to startup
		GOTO endSuccess
	) 

:installInStartup
	echo Adding vcam.exe to startup folder
	mklink "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\vcam.exe" "%~dp0\vcam.exe"
	if %errorLevel% == 0 (
		start  %~dp0vcam.exe
		echo.
	) else (
		echo failed adding to on startup
		echo.
		goto end
	)

:endSuccess
	echo Installation Completed Sucessfully.
	echo.

:end
	pause

