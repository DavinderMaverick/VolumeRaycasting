# VolumeRaycasting

## Transfer Function Only
![image](https://user-images.githubusercontent.com/50461188/57693295-7bf66d80-7666-11e9-90ca-2856252dc97c.png)

## + Shading
![image](https://user-images.githubusercontent.com/50461188/57693261-6c772480-7666-11e9-931c-1bc76324c99c.png)

## Usage
Press 1 -> Transfer Function Only  
Press 2 -> Shading Enabled  
Esc -> Quit  

Mouse Click and Drag -> Rotate View  
Mouse Scroll Wheel -> Zoom In/Out  

## Project Dependencies
1. opengl  
2. glad  
3. glfw  
4. glm  
5. stb  

## Using Different Volume Data

1. Delete the gradients.bin file.

2. Change the Input File Name 
https://github.com/DavinderMaverick/VolumeRaycasting/blob/14263af291a422a6abf03c8f9dd5a3b362b9f9ee/main.cpp#L110

3. These will be the list of transfer control points that we will setup and interpolate to produce the transfer function.

    You need to specify at least two control points at isovalues 0 and 256 for both alpha and color. Also the control points need to be ordered (low to high) by the isovalue. So the first entry in the list should always be the RGB/alpha value for the zero isovalue, and the last entry should always be the RGB/alpha value for the 256 isovalue.

    https://github.com/DavinderMaverick/VolumeRaycasting/blob/14263af291a422a6abf03c8f9dd5a3b362b9f9ee/main.cpp#L154-L169
