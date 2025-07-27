#version 330 core
// Layout:
layout(location = 0) in vec3 VertexPosition;

out vec3 TexCoords; 

uniform mat4 projection;
uniform mat4 view;

// Rotation Matrix
float angle = 40;
float c = cos(angle);
float s = sin(angle);

mat4 Rx = mat4( 
                1.0, 0.0, 0.0, 0.0, // First column
                0.0, c,   -s,   0.0, // Second column
                0.0, s,   c,   0.0, // Third column
                0.0, 0.0, 0.0, 1.0  // Fourth column
                );

mat4 Ry = mat4( 
                c,   0.0,  s,   0.0, // First column
                0.0, 1.0,  0.0, 0.0, // Second column
                -s,  0.0,  c,   0.0, // Third column
                0.0, 0.0, 0.0,  1.0  // Fourth column
                );

mat4 Rz = mat4( 
                c,   -s,   0.0, 0.0, // First column
                s,   c,    0.0, 0.0, // Second column
                0.0, 0.0,  1.0, 0.0, // Third column
                0.0, 0.0, 0.0,  1.0  // Fourth column
                );

void main()
{
    TexCoords = VertexPosition;



    // Get original camera rotation before inverse
    mat3 original_camera_rotation = transpose(mat3(view));
    mat4 camRot4x4 = mat4(original_camera_rotation);
    camRot4x4[3] = vec4(0.0, 0.0, 0.0, 1.0);
    // Get only camera orientatio without translation
    mat3 camera_orientation = mat3(view);
    mat4 cam = mat4(camera_orientation);

    gl_Position = projection * cam * vec4(VertexPosition, 1.0);
}  