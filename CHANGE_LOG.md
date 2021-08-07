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
- fixed cross ignoring position editing from properties window;
- removed local properties;
# 0.12.1
- fixed new object creation mode still being active when another tool is selected;
- changed steps for step settings;
- fixed Enter, Escape and other keys triggering combinations while in text input mode. Now - press Esc to deactivate any input widgets first and then press any combination;
# 0.13
- added sine curve drawing;
# 0.13.1
- introduced unique naming;
- fixed sine curve disappearing when rotation is nan;
- when switching between SinePen Step123 and Step132 the cross will move to the existant relevant point (optional);
- fixed delete all (Close) command leaving selected objects alive and rendered;
# 0.13.2
- fixed quaternion not being normalized on rotation;
- added dev documentation;
- configured project files for easier build and publishing. 
Now Release copies (CopyIfNewer) necessary files to output so the folder can be easily run from output or transferred to another pc and run there. 
Added Publish configuration that cleans output before build and copies only necessary files without any debug symbols;
# 0.13.3
- property/event overhaul;
# 0.14
- introduced independent cross movement and rotation;
- isolated main math in separate static classes;
- fixed sine building when 2 points are at the same location;
# 0.14.1
- fixed target mode locking;
- added combination Ctrl+A - select all scene objects;
# 0.14.2
- removed duplicate shaders;
- added tool tips;
# 0.15
- added context menu to scene object inspector;
# 0.15.1
- fixed Z axis when moving with mouse; 
# 0.16
- reworked change buffer;
- fixed issue with selecting incorrect object when selecting via inspector;
# 0.17
- added point object;
- added point pen tool;
- localized object names in inspector;
- made ./scenes start directory for file management;
- creates ./scenes directory if not exists;
# 0.18
- implemented color sum;
- implemented batch render (improved performance for mass rendering);