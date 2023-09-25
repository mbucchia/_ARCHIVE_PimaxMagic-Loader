@echo off
echo Reverting to original vrclient_x64.dll...
if exist real_vrclient_x64.dll (
	fc /b our_vrclient_x64.dll vrclient_x64.dll > nul 2>&1
	if errorlevel 1 (
		echo Detected changes to vrclient_x64.dll... Assuming SteamVR was updated.
		del real_vrclient_x64.dll
	) else (
		del vrclient_x64.dll
		ren real_vrclient_x64.dll vrclient_x64.dll
	)
) else (
	echo NOTHING TO UNINSTALL
)
pause
