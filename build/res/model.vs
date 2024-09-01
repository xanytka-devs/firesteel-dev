#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 frag_POS;
    vec2 frag_UV;
    mat3 tan_MATRIX;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {	
	vs_out.frag_POS = vec3(model * vec4(aPos, 1.0));
	vs_out.frag_UV = aUV;
	
	mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vs_out.tan_MATRIX = transpose(mat3(T, B, N));
	
	gl_Position = projection * view * vec4(vs_out.frag_POS, 1.0);
}