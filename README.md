# StereoOriginal

A 3d modelling and visualisation solution with stereo output (anaglyph) and multi pov view (camera user position tracking).

## Implemented instruments
- Polyline drawing
- Polyline extruding
- Object transformation (move, rotate, scale)

## Features
- Anaglyphical output
- Multi pov view via user position tracking
- Local, world coordinate systems
- Hierarchical object structure
- World transformation preservation
- Selecting objects
- Transforming/moving selected objects
- Change buffer. Ctrl+Z, Ctrl+Y
- Transformation tracing

![trace object rotation](https://github.com/prizrak9/StereoOriginal/blob/dev/docs/trace.png?raw=true)
![select some of the created clones and move them](https://github.com/prizrak9/StereoOriginal/blob/dev/docs/moveSelectedClones.png?raw=true)
![rotate what is left of traced object and rotate it to create a flower](https://github.com/prizrak9/StereoOriginal/blob/dev/docs/flower.png?raw=true)

## Known Issues
Combinations Shift + any Numpad key isn't working. It's a windows' "feature". https://github.com/glfw/glfw/issues/670