#version 330 core
out vec4 frag_COLOR;

in vec3 frag_POS;
in vec2 frag_UV;
in vec3 frag_NORMAL;

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	float emissionFactor;
	vec3 emissionColor;
	float shininess;
	float skyboxRefraction;
	float skyboxRefractionStrength;
};
uniform Material material;
uniform bool noTextures;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_emission1;
uniform int DrawMode;

struct PointLight {
	vec3 position;
	vec3 color;
};
uniform PointLight pointLight;
uniform vec3 viewPos;

float near = 0.1; 
float far  = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {

	// textures or material values
	vec4 diffMap = vec4(material.diffuse,1);
	vec4 specMap = vec4(material.specular,1);
	vec4 emisMap = vec4(material.emission,1);
	if(!noTextures) {
		diffMap = texture(texture_diffuse1, frag_UV);
		if(diffMap.a<0.5) discard;
		specMap = texture(texture_specular1, frag_UV);
		emisMap = texture(texture_emission1, frag_UV);
	}
	
	// ambient
    vec3 ambient = material.ambient * pointLight.color;
  	
    // diffuse 
    vec3 norm = normalize(frag_NORMAL);
    vec3 lightDir = normalize(pointLight.position - frag_POS);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * pointLight.color;
    
    // specular
    vec3 viewDir = normalize(viewPos - frag_POS);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = material.specular * spec * pointLight.color;
	
	// emission
    vec3 glow = (emisMap.xyz * material.emissionColor) * material.emissionFactor;
        
    vec3 result = (ambient + diffuse + specular + glow) * diffMap.xyz;
	
	if(DrawMode==0) frag_COLOR = vec4(result, 1.0);
	else if(DrawMode==1) frag_COLOR = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
}