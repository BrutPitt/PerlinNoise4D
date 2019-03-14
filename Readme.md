# PerlinNoise4D

### Volumetric Perlin Noise 4D 


![](https://raw.githubusercontent.com/BrutPitt/PerlinNoise4D/master/screenShots/noise4D.gif)


This is my 2005 (2D texture sliders) / 2007 (3D textute) C/C++ multi-threading (OpenMP) exeriment, with OpenGL texture sliders view. (switch from 2D/3D textures via defines)

It uses OpenGL 1.5 (old style) version (e.g. glVertex/glColor/etc) and only texture sliders transparence (without glsl shaders)

I just added a recent Visual Studio 2017 project solution, but it is portable and can be compiled under other O.S. 

To build need **glut**/**freeglut** tool, and **OpenMP** if you want use multithreading (you can enable/disable it via defines).
