#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 frag_POS;

uniform mat4 projection;
uniform mat4 view;

void main() {
    frag_POS = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}