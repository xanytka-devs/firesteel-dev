#version 330 core

layout (location = 0) in vec3 aPos;

uniform vec3 offset;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
	vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp = vec3(view[0][1], view[1][1], view[2][1]);

    vec4 posOffset = vec4(aPos + offset, 1.0);
	vec4 worldPosition = model * vec4(0.0, 0.0, 0.0, 1.0);
    vec3 position = worldPosition.xyz + posOffset.x * camRight + posOffset.y * camUp;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
