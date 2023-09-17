@echo off
echo Injecting PimaxMagic-Loader...
if exist real_vrclient_x64.dll (
	echo A COPY OF real_vrclient_x64.dll IS ALREADY IN THE FOLDER.
) else (
	ren vrclient_x64.dll real_vrclient_x64.dll
	copy our_vrclient_x64.dll vrclient_x64.dll
)
pause
