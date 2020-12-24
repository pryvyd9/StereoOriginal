# 0.9.0
- switched red and cyan colors;
- updated ImGui to 1.80 docking;
- updated glfw to 3.2.2;
- disabled ImGui arrow navigation;
- added transition, rotation, scale step configuration;
- removed overcomplicated mouse control;
- used ImGui keystatus logic;
- implemented discrete movement;
- average deltatime + framerate;
- reversed Z;
# 0.10
- added color settings;
- moved logs to file;
- moved to World center-centered Millimeter based coordinate system;
- user position is calculated based on the screen center position;
# 0.11
- added scene window transparency;
# 0.12
- reworked hotkeys. Now hotkeys without modifiers(Ctrl, Alt) won't be triggered while typing text. Modifier Shift is ignored while in text inputting state;
- fixed the issue with Pen tool removing more last vertices than expected;
- reworked control. Now to use mouse control - drag scene image. Also, depending on the desired action (translate, rotate, scale) - use (Alt, Ctrl, Shift) respectively;
- objects can be rotated around 2 axes simultaneously;
- enabled translation in local space for multiple objects. They are being moved in respect to cross rotation;

! in windows, Shift is overlapping with numpad keys so combinations Shift+Numpad aren't possible https://github.com/glfw/glfw/issues/670