R""(
#version 410 core
precision highp float;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

uniform float size;

out vec2 v_texCoord;

void main()
{
	gl_Position = vec4(position * size, 0.0, 1.0);
	v_texCoord = texCoord;
}
)""
