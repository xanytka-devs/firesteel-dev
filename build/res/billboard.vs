#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 frag_UV;

void main() {
    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp = vec3(view[0][1], view[1][1], view[2][1]);

    vec4 worldPosition = model * vec4(0.0, 0.0, 0.0, 1.0);
    vec3 position = worldPosition.xyz + aPos.x * camRight + aPos.y * camUp;

    gl_Position = projection * view * vec4(position, 1.0);
    frag_UV = aUV;
}
