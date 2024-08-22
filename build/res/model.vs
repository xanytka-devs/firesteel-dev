#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out vec3 frag_POS;
out vec2 frag_UV;
out vec3 frag_NORMAL;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {	
	frag_POS = vec3(model * vec4(aPos, 1.0));
	frag_NORMAL = mat3(transpose(inverse(model))) * aNormal;
	frag_UV = aUV;
	
	gl_Position = projection * view * vec4(frag_POS, 1.0);
}