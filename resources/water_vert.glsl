#version 330

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;

out vec3 light_vector;
out vec3 normal_vector;
out vec3 halfway_vector;
out float fog_factor;

void main() {
	gl_Position = V * M * vec4(vertPos, 1.0);
	fog_factor = min(-gl_Position.z/500.0, 1.0);
	gl_Position = P * gl_Position;

	vec4 v1 = V * M * vec4(vertPos, 1.0);
	vec3 normal1 = normalize(vertNor);

	light_vector = normalize((V * vec4(lightPos, 1.0)).xyz - v1.xyz);
	normal_vector = (inverse(transpose(V * M)) * vec4(normal1, 0.0)).xyz;
        halfway_vector = light_vector + normalize(-v1.xyz);
}