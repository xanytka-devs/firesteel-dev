#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

out VS_OUT {
    vec3 view_POS;
    vec3 frag_POS;
    vec2 frag_UV;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

void main() {
	vs_out.frag_POS = vec3(model * vec4(aPos, 1.0));
	vs_out.frag_UV = aUV;
	vs_out.view_POS = viewPos;
	
	gl_Position = projection * view * vec4(vs_out.frag_POS, 1.0);
}