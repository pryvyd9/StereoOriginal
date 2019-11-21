#version 330 core
layout(location = 0) out vec3 color;
uniform float farT;
uniform float closeT;
uniform bool isZero;
out vec4 FragColor;
void main()
{
	if (isZero || farT > gl_FragCoord.z && gl_FragCoord.z > closeT) 
		FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	else 
		FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
}