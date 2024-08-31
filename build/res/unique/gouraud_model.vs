#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out vec3 frag_POS;
out vec2 frag_UV;
out vec3 frag_NORMAL;
out vec3 frag_LIGHT;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

struct Material {
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	float emissionFactor;
	vec4 emissionColor;
	float shininess;
	float skyboxRefraction;
	float skyboxRefractionStrength;
};
uniform Material material;

struct PointLight {
	vec3 position;
	vec3 color;
};
uniform PointLight pointLight;
uniform vec3 viewPos;

void main() {	
	frag_POS = vec3(model * vec4(aPos, 1.0));
	frag_NORMAL = mat3(transpose(inverse(model))) * aNormal;
	frag_UV = aUV;
	
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
        
    frag_LIGHT = (ambient + diffuse + specular);
	
	gl_Position = projection * view * vec4(frag_POS, 1.0);
}