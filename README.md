# Order-Independent-Trasparency
Linked List Based Implementation

1. Basic Implementation

This is implementation of OIT based on AMD demo.
It created per fragment based link list.
In resolve pass we traverse the linked list per pixel.
Sort the linked list.
Compute final color for pixel.

2. Concepts Covered

This demo includes concept from openGL like.
Buffer Objects, Texture Objects, Image load and store operations.
Using image as memory to create linked list.
Atomic counters.
early_fragment_tests attribute in fragment shader.
Storing floating points numbers as bit pattern in uint and typcasting it back.
Pixel Buffer Objects to initialize texture.

3. Dependencies

The demo only uses opengl libraries.
TinyObj for obj loader. This is included in header file.
Glut for platform dependednt code like input and event handling.
All files are included with demo in Include and Lib folders.
Just for simplicity add dll in Debug folder once you build the project.

4. Building project
Just download the code .
Open Sln file and build visual studio project.
This demo is built using VS2012.

![alt tag](https://github.com/PixelClear/Order-Independent-Trasparency/blob/master/OIT.png)

Youtube Demo:

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/ktaFRpAgSno/0.jpg)](https://www.youtube.com/watch?v=ktaFRpAgSno)
