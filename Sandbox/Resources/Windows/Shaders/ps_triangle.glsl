 #version 330 core
 in vec3 fragColor;   // Interpolated color from the vertex shader
 out vec4 color;      // Output color to the framebuffer

 void main()
 {
     color = vec4(fragColor, 1.0); // Set fragment color
 }
