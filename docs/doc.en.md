# StereoOriginal User documentation
## GUI
GUI is designed using ImGui library https://github.com/ocornut/imgui.
### Tool window
Displays all tools available and some toggles: Local/World space mode, Discrete movement mode.
Clicking on tool button activates the tool.
### Attributes window
### Inspector window
Displays all scene objects in the scene. 
All objects are child to the root object. 

Implemens selecting and dragging objects in the object hierarchy. 
Objects can be selected by pressing LMB while hovering mouse on object name and to the right of it.

Objects can be expanded to see their children by pressing on the triangle to the left of the object name. 
If there's a point then the objects doesn't have any child objects. 
An exception is Rool object. 
It has a point but it's just to forbid it's minimizing.

Several objects can be selected by modifying LMB with Ctrl. 
A tree of objects can be selected by modifying LMB with Alt.

Click RMB on an item to open a context menu.
Context menu has 2 items: Select tree, Select children.
### Properties window
### Settings window
- Buffer size;
- Language: English, Ukrainian;
- Step: Translation, rotation, scale, mouse sensivity;
- Color: LeftBright, RightBright, LeftDim, Right Dim;
- Log file name;
- PPI;
- Scene window transparency (0.0-1.0);
### Scene window
Displays current scene rendered in anaglyph mode. 
Any action conducted on scene objects are seen in this window. 

Can be transparent. 
The transparency level is set in settings. 
If the window is docked, the objects behind main window are seen. 
If it's detached, only objects behind this window are seen.

Image size is scaled by PPI setting. With correct PPI set the millimeter on screen should equal the millimeter in scene.
### File window
## IO
### Hotkeys
Here are listed all common combinations. 
Tools override some of then and add new ones. 
For more details see Tools.
- Esc - exit text inputting mode. Remove focus from all widgets. When no widget is active - deactivates tools;
- Ctrl+Z - undo;
- Ctrl+Y - redo;
- Ctrl+A - select all scene objects;
- Ctrl+D - deselect all scene objects;
- Q - toggle discrete movement mode;
- W - switch space mode (Local/World);
- C - switch target mode (Object/Pivot);
- T - transformation tool;
- P - pen tool;
- E - extrusion tool;
- F5 - save rendered scene to file;
- F6 - render a scene in 4000x4000 resolution and save to file;
### Cross control
Cross is controlled with a keyboard and a mouse. This behaviour is overriden by tools.

Movement from keyboard is scaled by a set step in settings. Movement from mouse is scaled by mouse sensivity, set in settings and depends on mouse movement speed.
- Alt+Arrow key to move in XY plane;
- Alt+Numpad arrow key to move in XY plane;
- Alt+Numpad1 to move closer to user (+Z);
- Alt+Numpad9 to move further from user (-Z);
- Numpad7/Numpad3 to resize cross;
- Alt+LMB on scene+Mouse movement to move in XY plane;
- Alt+LMB on scene+RMB+Mouse movement to move in Z axe;
### File
## Render
Rendering is implemented with OpenGL 4.1+
## Tools
Pressing Esc deactivates tools.
### Pen
When no object is selected, or 1 object is selected that is not a polyline Pen enteres Object Creation mode(OCM).
While in OCM - press Enter to create a new polyline object. New objects are created at Cross' location.
Deselecting polyline or selecting non-polyline object will enter OCM.
New objects are created at the same level as selected object or in Root.

Polyline is drawn by moving Cross.

#### Immediate mode
IM creates new poins on its own while Cross is moved. 
Pressing Enter will unbind current polyline and enter OCM.
#### Step mode
SM requires pressing Enter to fix the point and create a new one.

### SinePen
Operates the same as Pen SM.
ShouldMoveCrossOnSinePenModeChange option determines if the cross' position will be set to the relevant vertice on mode change.

#### Step123
Second point is (0,1) for cosine.
#### Step132
Tried point is (0,1) for cosine.

### Transformation

When multiple objects are selected cross' rotation is equal to the first object in selection and position is equal to mean position of all selected objects. 
See TransformTool::OnSelectionChanged for more details.

### Extrusion