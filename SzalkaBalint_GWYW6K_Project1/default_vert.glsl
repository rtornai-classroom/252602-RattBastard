#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform vec2 uOffset;

void main()
{
    gl_Position = uProjection * vec4(aPos + uOffset, 0.0, 1.0);
}
