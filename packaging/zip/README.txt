VortexPanes - Snow on your Windows desktop
==========================================

Requirements
------------
Windows 11 (version 21H2 or later). Nothing else needs to be installed:
the folder contains everything the application needs.

Start
-----
1. Extract this zip to any folder (e.g. C:\Tools\VortexPanes).
2. Run VortexPanes.exe. Keep all files of the folder together; the
   settings window uses the libraries in this folder. A snowflake icon
   appears in the notification area.
3. Right-click the icon: "Settings" shows app info, "Exit" closes the app.
   A click on the icon also opens the settings.

Windows SmartScreen
-------------------
This build is not code-signed. On first start Windows may show
"Windows protected your PC". Click "More info" -> "Run anyway".
Verify the download with SHA256SUMS.txt from the GitHub release page.

Uninstall
---------
Exit the app via the tray icon,
delete the folder and %AppData%\VortexPanes.

Which zip do I need?
--------------------
x64   - normal Intel/AMD PCs
arm64 - ARM devices (Snapdragon, Surface Pro X, ...)
x86   - old 32-bit Windows installations

License
-------
VortexPanes is released under the MIT License (LICENSE.txt).
The folder also contains components of the Microsoft Windows App SDK
(Microsoft.*.dll and the other runtime files of the settings window).
They are redistributed under the Microsoft Software License Terms in
licenses\Microsoft.WindowsAppSDK-LICENSE.txt, which apply to their use.
Further notices: THIRD_PARTY_NOTICES.txt and the licenses folder.

Source, issues, license (MIT): https://github.com/lk-code/vortex-panes
