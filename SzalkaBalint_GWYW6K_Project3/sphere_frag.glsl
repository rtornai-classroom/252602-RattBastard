#version 330 core

out vec4 FragColor;

uniform vec3 emissiveColor;
uniform int  lightOn;

void main()
{
    if (lightOn == 1)
        FragColor = vec4(emissiveColor, 1.0);
    else
        FragColor = vec4(0.22, 0.22, 0.22, 1.0);
}
