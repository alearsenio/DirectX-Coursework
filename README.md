# DirectX-Coursework

Description:
For one of the modules of my MSc in Computer Games Technology, I had to develop a simple game using C++ and the DirectX libraries,
which could summarise what I learned during the year about the render pipeline, shaders, procedural methods, and how to work on a game engine build from scratch

Features of the game:
In the game, the player controls a submarine which has to navigate and destroy, by shooting missiles, the watermines hidden under the water surface.
The most remarkable features implemented in the engine I worked with were:
- Skybox: a cube map used as skybox to fill the background of the scene
- Skybox Reflection: a custom shader to reflect the skybox on the surface of reflective objects and of the water
- Procedural Terrain: a simplex noise and FBM algorithm on CPU to procedurally change the height map of a terrain to recreate mountains and the seafloor
- Procedural water's waves: a simplex noise algorithm on GPU to recreate the waves movement on the surface of the water
- Post Processing: a post processing effect to add motion blur and recreate the effect of the camera underwater
(more insights of the development can be found here)

The experience:
While working on this project, I understood how satisfying is to build the graphic of a game from scratch,
seeing an engine evoles from rendering simple geometries to procedurally moves the surface of water and reflect the sky.
I also understood the power of procedural methods, and how these techinques can be a robust and easy alternative to modelling or animating an object manually.
Lastly, it became clearer to me the strength of shader programming, the performance improvements which can be achieved when computing on GPU
and the vastness of effects which can be generated using shaders. 
