@echo off
echo Reverting to original vrclient_x64.dll...
if exist real_vrclient_x64.dll (
	del vrclient_x64.dll
	ren real_vrclient_x64.dll vrclient_x64.dll
) else (
	echo NOTHING TO UNINSTALL
)
pause
