// Resource identifiers shared by app.rc, the language files and the C++ code.
#ifndef VP_RESOURCE_H
#define VP_RESOURCE_H

// Icons (IDI_APP must have the lowest ID so that it becomes the executable icon)
#define IDI_APP 1
#define IDI_TRAY_LIGHT 2
#define IDI_TRAY_DARK 3

// Strings
#define IDS_TRAY_TOOLTIP 100
#define IDS_MENU_SETTINGS 110
#define IDS_MENU_EXIT 111
#define IDS_ERROR_STARTUP 120
#define IDS_ERROR_GRAPHICS 121
#define IDS_ERROR_TRAY_ICON 122

// Menu commands
#define IDM_SETTINGS 1001
#define IDM_EXIT 1002

#endif // VP_RESOURCE_H
