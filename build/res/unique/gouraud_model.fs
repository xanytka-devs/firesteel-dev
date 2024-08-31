#version 330 core
out vec4 frag_COLOR;

in vec3 frag_POS;
in vec2 frag_UV;
in vec3 frag_NORMAL;
in vec3 frag_LIGHT;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_height1;
uniform int DrawMode;
uniform bool noTextures;
struct Material {
	vec4 diffuse;
	vec4 specular;
	vec4 emission;
	float emission_factor;
	vec4 emission_color;
	float shininess;
	float skybox_refraction;
	float skybox_refraction_strength;
};
uniform Material material;

float near = 0.1; 
float far  = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
	// textures or material values
	vec4 diffMap = material.diffuse;
	if(!noTextures) {
		diffMap = texture(texture_diffuse1, frag_UV);
		if(diffMap.a<0.5) discard;
	}
	if(diffMap.a<0.5) discard;

    vec3 result = frag_LIGHT * diffMap.xyz;
	
	if(DrawMode==0) frag_COLOR = vec4(result, 1.0);
	else if(DrawMode==1) frag_COLOR = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
}