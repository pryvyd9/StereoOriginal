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
- fixed ObjectSelection sending 2 events on Set();
- fixed IsDown/IsPressed(<key>, false) methods working the other way around;
- added shortcut Ctrl+D for deselect all;
- fixed pen drawing in deleted selected object;
- fixed tracing only tracing the original object;
- fixed pen thinking the line continuing to be drawn in straight line when it's drawn in the opposite direction;