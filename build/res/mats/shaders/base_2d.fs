#version 330 core
layout (location = 0) out vec4 frag_COLOR;

in VS_OUT {
    vec3 view_POS;
    vec3 frag_POS;
    vec2 frag_UV;
} fs_in;

uniform int drawMode;
uniform bool noTextures;
uniform vec3 color;
uniform sampler2D diffuse0;

void main() {
	vec4 diffMap = vec4(color, 1);
	if(!noTextures) {
		diffMap = texture(diffuse0, fs_in.frag_UV);
	}
	
	frag_COLOR = diffMap;
}