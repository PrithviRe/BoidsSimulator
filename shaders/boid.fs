#version 330 core
in vec3 FragColorTint;
out vec4 FragColor;

void main() {
    FragColor = vec4(FragColorTint, 1.0);
}

