#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform vec2 uCenter;

out vec2 vLocalPos;

void main()
{
    vLocalPos = aPos;
    gl_Position = uProjection * vec4(aPos + uCenter, 0.0, 1.0);
}
