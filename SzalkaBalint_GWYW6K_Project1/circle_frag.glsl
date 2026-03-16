#version 330 core

in vec2 vLocalPos;
out vec4 FragColor;

uniform float uRadius;

void main()
{
    float dist = length(vLocalPos);

    if (dist > uRadius) discard;

    float t = dist / uRadius;
    vec3 color = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), t);

    FragColor = vec4(color, 1.0);
}
