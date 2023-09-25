# Pimax Foveated Rendering Loader for Easy Anti-Cheat

## What does it do?

This utility lets you use the Pimax Foveated Rendering feature with games protected with Easy Anti-Cheat.

# More details and instructions on the [the wiki](https://github.com/mbucchia/PimaxMagic-Loader/wiki)!

## Installing

**PLEASE READ ALL THE INSTRUCTIONS BELOW CAREFULLY.**

1) Install [Pimax Play](https://pimax.com/pimax-pc) from the Pimax website and set it up for Eye Tracking and Foveated Rendering as usual.

2) Download the Pimax Magic Loader from the [Releases](https://github.com/mbucchia/PimaxMagic-Loader/releases) page.

3) Locate the folder where SteamVR is installed. It typically is `C:\Program Files (x86)\Steam\steamapps\common\SteamVR`. Go to the `bin` folder under the SteamVR folder. You should see files named `vrclient.dll` and `vrclient_x64.dll` (**if not, then you are in the wrong folder**).

4) With **SteamVR completely closed**, unzip the Pimax Magic Loader (obtained in step 2) directly into the `bin` folder from step 4).

5) Run `install_pimaxmagic_loader.bat`. The successful output will show:

```
Injecting PimaxMagic-Loader...
        1 file(s) copied.
Press any key to continue . . .
```

## Uninstalling and dealing with SteamVR updates

To make the program inactive and uninstall it:

1) Locate the folder where SteamVR is installed. It typically is `C:\Program Files (x86)\Steam\steamapps\common\SteamVR`. Go to the `bin` folder under the SteamVR folder.

2) With **SteamVR completely closed**, run `uninstall_pimaxmagic_loader.bat`. The successful output will show:

```
Reverting to original vrclient_x64.dll...
Press any key to continue . . .
```

After a SteamVR update, it is necessary to re-activate the software:

1) Locate the folder where SteamVR is installed. It typically is `C:\Program Files (x86)\Steam\steamapps\common\SteamVR`. Go to the `bin` folder under the SteamVR folder.

2) With **SteamVR completely closed**, run `uninstall_pimaxmagic_loader.bat`. The successful output will show:

```
Reverting to original vrclient_x64.dll...
Detected changes to vrclient_x64.dll... Assuming SteamVR was updated.
Press any key to continue . . .
```

3) With **SteamVR completely closed**, run `install_pimaxmagic_loader.bat`. The successful output will show:

```
Injecting PimaxMagic-Loader...
        1 file(s) copied.
Press any key to continue . . .
```

## Frequently Asked Questions (FAQ)

**Q:** Does it work with all games?

**A:** I don't know. This program does not affect Pimax foveated rendering solution. There are certain games that simply cannot do foveated rendering with Pimax. This tool was primarily developed to enable Pimax foveated rendering in VRChat and was only tested with VRChat.

**Q:** SteamVR does not start anymore.

**A:** Perhaps some files were corrupted. Try doing a repair/reinstall of SteamVR from Steam.

## Troubleshooting

The log files are in the `%LocalAppData%\PimaxMagic-Loader` folder.

## Special Thanks

Huge thanks to [Omniwhatever](https://www.youtube.com/@Omniwhatever) for getting me to look into doing this and helping me brainstorming solutions and testing prerelease versions!

Very special thanks to Pimax for providing me with a Pimax Crystal for development.

