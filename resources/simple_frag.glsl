#version 330 core

uniform vec3 lightPos;
uniform vec3 eye;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float shine;

in vec3 normal;
in vec3 vertPosition;
in vec3 eyePos;
out vec3 color;

const vec3 maxIntensity = vec3(1.0, 1.0, 1.0);

void main()
{
	vec3 light = lightPos - vertPosition;
	light = normalize(light);

	vec3 eyeDir = normalize(eye - vertPosition);
	vec3 H = normalize(light + eyeDir);

	vec3 nNormal = normalize(normal);

	vec3 diffLight = MatDif * clamp(dot(nNormal, light), 0.0, 1.0) * maxIntensity;
	vec3 ambLight = MatAmb * maxIntensity;
	vec3 specLight = MatSpec * pow(dot(nNormal, H), shine) * maxIntensity;
 
	color = clamp(diffLight + ambLight + specLight, 0, 1);
}