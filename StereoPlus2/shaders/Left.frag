#version 330 core
layout(location = 0) out vec3 color;

uniform bool isZero;
uniform bool isX;

uniform float min;
uniform float max;

out vec4 FragColor;

void main()
{
	FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
}