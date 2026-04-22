#version 330 core

layout(location = 0) in vec2 position;

uniform vec2 resolution;

void main()
{
    vec2 ndc = (position / resolution) * 2.0 - 1.0;
    ndc.y = -ndc.y;

    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = 5.0;
}