#version 330 core

uniform int  isLine;
uniform vec4 color;

out vec4 fragColor;

void main()
{
    if (isLine == 0) {
        vec2 coord = gl_PointCoord - vec2(0.5);
        if (length(coord) > 0.5)
            discard;
    }

    fragColor = color;
}
