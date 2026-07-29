#version 450

layout(location = 0) in vec2 fragWorldXZ;
layout(location = 0) out vec4 outColor;

const float cellSize = 1.0;
const float lineWidth = 0.02;

void main()
{
    vec2 cellCoords = fract(fragWorldXZ / cellSize);

    vec2 distanceToEdge = min(cellCoords, 1.0 - cellCoords);

    float distanceToLine =
        min(distanceToEdge.x, distanceToEdge.y);

    float lineFactor =
        1.0 - smoothstep(0.0, lineWidth, distanceToLine);

    vec3 backgroundColor = vec3(0.03, 0.04, 0.05);
    vec3 gridColor = vec3(0.35, 0.40, 0.45);

    vec3 color = mix(
        backgroundColor,
        gridColor,
        lineFactor
    );

    outColor = vec4(color, 1.0);
}