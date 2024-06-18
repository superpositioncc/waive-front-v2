R""(
#version 410 core
precision highp float;

uniform float value;

in vec2 v_texCoord;

out vec4 color;

void main()
{
    color = vec4(v_texCoord.x, v_texCoord.y, value, 1.0);
}
)""
