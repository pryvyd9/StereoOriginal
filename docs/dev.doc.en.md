# StereoOriginal Developer documentation
## Build
The only architecture included is x64.
It's because included GLFW and OpenCV dlls are only x64 versions so to build x86 it is required to download and link x86 versions of their dlls.

Majority of the files in the project are .hpp. 
It's done to ease referencing files and fight compiler bugs of not being able to detect windows platform for x64.
To reproduce it - change ClInclude to ClCompile for Key.hpp.
It doesn't happen for all files but somehow making files compilable doesn't make then compile on build.

This means that when a change is performed in hpp files (basically any change), you need to Rebuild All the solution to apply the changes.

### Building profiles
- Debug. Builds debug symbols, executable and ilk files;
- Release. Same as debug + uses Maximum speed optimization and copies (CopyIfNewer) all necessary files for running the app from the folder.
- Publish. Cleans output folder before build. Builds executable and copies all necessary files for running the app from the folder.
Uses Maximum speed optimization.

## Evolution
### Architecture
The initial idea of architecture was DDD + functional approach.
Entities had no logic and all behavioural classes had Init, Process, Exit stages.
All the methods had to return something and result of Init, Process, Exit determines wether the application should continue (true) or stop (false).
It's a custom graceful shutdown idea.

As the number of components and rendering complexity grew it ws decided to scrap the whole DDD + functional approach.
Now, the target architecture is OOP + State machine.
All the scene objects have methods to draw themselves, they have vertice, connection, index etc. caches to speedup rendering.
Scene objects also have a lot of vertual methods to be overriden such as vertices manipulation, property editing and event handling.

A lot of classes that shouldn't be instanced do have them because of the initial idea to have everything separated to allow for multiple scene processing in parallel.
The idea was rejected later but the transition of the tools to state machine architecture hasn't been completed.

### Object hierarcy
The original object hierarchy was imagined as local coordinate structure where object's world coordinates were calculated from combining transformations of ancestors.
It was implemented and then scraped since it hadn't been requested and then nobody was interested in it.
It also proved not being very effective so transformation procedures were reworked in favor of world-first coordinate system where everything is stored in world coordinates but local coordinate transformation is possible.
The cascade transformation system still exists as it has legacy dependencies and thanks to lack of capacity to actually remodel it and retest the whole system again.

## Dependencies
- OpenGL 4.1+
- OpenCV
- ImGui 1.80 docking branch (1)
- GLM
- GLFW 3.2.2
- GL3W
- stb (include/stb/stb_image_write)

All the dependencies are provided in the repository except OpenGL.

(1) modified to allow for better transparency handling.

## Module desctiption
### Path
Path is for better C++17 file managing API.
Transforms strings to paths back and forth and merges them.

### Time
Time is for frameDeltaTime and framerate measuring.
Also gets current time and formatts time for log output.

### Log
Log is for wrinting messages to console window.
Can be extended with additional sinks to write to file or other places.
Formats and joins basic types and glm::vec3.

### Command
Command is .NET Task alternative.
Can be fired but cannot be awaited.
Has many derived classes best suited for different tasks.

An enclosed system create and scheduled on instantiating and executed at the end of frame.
Memory is released after command execution.

Commands can be ready to run on instantiation or toggled later depending on command.
Commands are executed when ready.

All scene objects are created and deleted via commands to prevent following after scene object list modification frame logic disruption.

Commands implement simple asyncrony mechanism.

### Event
Event is an attempt to .NET event system.
Event must be stored as private field and exposed as IEvent via additional get method.
Handlers are appended and removed via methods or overloaded operators.

### Property
Property contains value and implements OnChanged event.
Properties can be bind one/two way to syncronize values.
Binding two way makes 2 properties reference single node holding value.
Has readonly implementation and static property macros.

### Input
Input is for Keyboard + Mouse input.
Implements key combination shortcuts.
Uses ImGui key repetition. Both repetitive and constant flow IsPressed are implemented.
Implements Continuous input which returns true while any key is pressed.
Implements Continuous1SecondDelay input which returns true while any key is pressed or no key pressed but the last key was pressed withing 1 second.
Implements OnFrame handlers that are executed each frame.

### FileManager
FileManager is for storing/reading settings, project files etc. in binary and json text modes.
Implements custom Json serialization/deserialization in Json.hpp.

### Settings
Settings presents global state and defines global settings and flags that can be changed in runtime and their change triggers coordinate space mode, language, transformation steps etc.

### TemplateExtensions
An experimental module that was intended to used to merge a lot of repetitive code but the idea was scraped due to poor IDE support not being able to analyse what is going on in these templates and therefore reducing readability greatly.

### Localization
Provides relevant localization as quickly as possible.

### Render
Receives a collestion of scene objects and draw them either dim or bright depending on their selections status.

### Windows
Middlemen between GUI and tools/objects