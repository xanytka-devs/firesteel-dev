#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 view_POS;
    vec3 frag_POS;
    vec2 frag_UV;
    vec3 frag_NORMAL;
    vec3 frag_TAN;
    vec3 frag_BITAN;
    mat3 tan_MATRIX;
	float fog_FACTOR;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 fogPosition;
uniform vec3 viewPos;

void main() {	
	vs_out.frag_POS = vec3(model * vec4(aPos, 1.0));
	vs_out.frag_UV = aUV;
	vs_out.frag_NORMAL = aNormal;
	vs_out.frag_TAN = aTangent;
	vs_out.frag_BITAN = aBitangent;
	
	mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vs_out.tan_MATRIX = transpose(mat3(T, B, N));
	
	vs_out.view_POS = viewPos;
	// Calculate linear fog.
    float fogStart = fogPosition.x;
    float fogEnd = fogPosition.y;
	
	gl_Position = projection * view * vec4(vs_out.frag_POS, 1.0);
	vs_out.fog_FACTOR = clamp((fogEnd - (viewPos.z)) / (fogEnd - fogStart), 0.0f, 1.0f);
}