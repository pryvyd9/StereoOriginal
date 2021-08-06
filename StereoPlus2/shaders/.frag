#version 330 core
layout(location = 0) out vec3 color;

uniform vec4 myColor;

out vec4 FragColor;

void main()
{
	FragColor = myColor;
}