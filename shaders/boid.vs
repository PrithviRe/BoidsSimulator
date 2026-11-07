#version 330 core
layout (location = 0) in vec2 aMeshPos;
layout (location = 1) in vec2 aInstancePos;
layout (location = 2) in float aInstanceAngle;
layout (location = 3) in vec3 aInstanceColor;

out vec3 FragColorTint;

void main() {
    float c = cos(aInstanceAngle);
    float s = sin(aInstanceAngle);

    // Rotate local arrow mesh to face velocity direction
    vec2 rotatedPos = vec2(
        aMeshPos.x * c - aMeshPos.y * s,
        aMeshPos.x * s + aMeshPos.y * c
    );

    // Translate to instance position
    vec2 worldPos = rotatedPos + aInstancePos;

    gl_Position = vec4(worldPos, 0.0, 1.0);
    FragColorTint = aInstanceColor;
}

