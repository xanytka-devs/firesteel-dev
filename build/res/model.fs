#version 330 core
layout (location = 0) out vec4 frag_COLOR;
layout (location = 1) out vec4 frag_BRIGHTNESS;

in VS_OUT {
    vec3 view_POS;
    vec3 frag_POS;
    vec2 frag_UV;
    vec3 frag_NORMAL;
    vec3 frag_TAN;
    vec3 frag_BITAN;
    mat3 tan_MATRIX;
} fs_in;

struct Material {
	sampler2D diffuse0;
	vec4 diffuse;
	sampler2D specular0;
	vec4 specular;
	sampler2D emission0;
	vec4 emission;
	sampler2D normal0;
	float normalStrength;
	bool opacityMask;
	sampler2D opacity0;
	
	float emissionFactor;
	vec3 emissionColor;
	float shininess;
	float skyboxRefraction;
	float skyboxRefractionStrength;
};
uniform Material material;
uniform bool noTextures;
uniform int DrawMode;

struct DirectionalLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
struct PointLight {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
struct SpotLight {
	vec3 position;
	vec3 direction;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	
	float cutOff;
	float outerCutOff;
};

uniform DirectionalLight dirLight;

#define MAX_POINT_LIGHTS 32
uniform int numPointLights;
uniform PointLight[MAX_POINT_LIGHTS] pointLights;

#define MAX_SPOT_LIGHTS 32
uniform int numSpotLights;
uniform SpotLight[MAX_SPOT_LIGHTS] spotLights;

float near = 0.1; 
float far  = 100.0;
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

uniform int lightingType;
uniform samplerCube skybox;
vec3 calcDirLight  (vec4 diffMap, vec4 specMap, vec4 emisMap, vec4 normMap);
vec3 calcPointLight(int pLightIndex, vec4 diffMap, vec4 specMap, vec4 emisMap, vec4 normMap);
vec3 calcSpotLight (int sLightIndex, vec4 diffMap, vec4 specMap, vec4 emisMap, vec4 normMap);

uniform struct FogParameters {
	vec3 vFogColor; // Fog color
	float fStart; // This is only for linear fog
	float fEnd; // This is only for linear fog
	float fDensity; // For exp and exp2 equation
	
	int iEquation; // 0 = linear, 1 = exp, 2 = exp2
} fogParams;

float getFogFactor(float fFogCoord) {
	float fResult = 0.0;
	if(fogParams.iEquation == 0)
		fResult = (fogParams.fEnd-fFogCoord)/(fogParams.fEnd-fogParams.fStart);
	else if(fogParams.iEquation == 1)
		fResult = exp(-fogParams.fDensity*(fFogCoord-fogParams.fStart));
	else if(fogParams.iEquation == 2)
		fResult = exp(-pow(fogParams.fDensity*(fFogCoord-fogParams.fStart), 2.0));
		
	fResult = 1.0-clamp(fResult, 0.0, 1.0);
	
	return fResult;
}

vec3 TangentLightPos;
vec3 TangentViewPos;
vec3 TangentFragPos;

void main() {
	// material values
	vec4 diffMap = material.diffuse;
	vec4 opacMap = vec4(1);
	float transparency = 1.0;
	vec4 specMap = material.specular;
	vec4 emisMap = material.emission;
	vec4 normMap = vec4(1);
	// texture values
	if(!noTextures) {
		diffMap = texture(material.diffuse0, fs_in.frag_UV) * material.diffuse;
		opacMap = texture(material.opacity0, fs_in.frag_UV);
		transparency = diffMap.a;
		if(transparency<0.1) discard;
		if(material.opacityMask) transparency = opacMap.r;
		specMap = texture(material.specular0, fs_in.frag_UV);
		emisMap = texture(material.emission0, fs_in.frag_UV);
		normMap = texture(material.normal0, fs_in.frag_UV) * material.normalStrength;
	}
	// tangets
	TangentViewPos = fs_in.tan_MATRIX * fs_in.view_POS;
	TangentFragPos = fs_in.tan_MATRIX * fs_in.frag_POS;
	// normal
	// transform normal vector to range [-1,1]
    vec4 norm = normalize(normMap * 2.0 - 1.0);  // this normal is in tangent space
	// skybox
	vec3 I = normalize(fs_in.frag_POS - fs_in.view_POS);
	vec3 R = refract(I, normalize(fs_in.frag_NORMAL), material.skyboxRefraction);
	vec3 skyboxCalc = texture(skybox, -R).rgb * material.skyboxRefractionStrength;
	if(DrawMode!=9) skyboxCalc *= specMap.rgb;
	// light calculations
	vec3 result = vec3(0);
	if(lightingType > 0) {
		// directional light
		result = calcDirLight(diffMap, specMap, emisMap, norm);
		// point lights
		for(int p = 0; p < numPointLights; p++)
			result += calcPointLight(p, diffMap, specMap, emisMap, norm);
		// spot lights
		for(int s = 0; s < numSpotLights; s++)
			result += calcSpotLight(s, diffMap, specMap, emisMap, norm);
		// emission
		result += (emisMap.rgb * material.emissionColor) * material.emissionFactor;
		// skybox
		result += skyboxCalc;
	} else result = diffMap.rgb;
	// fog
	float fFogCoord = abs(gl_FragCoord.z/gl_FragCoord.w);
	result = mix(result, fogParams.vFogColor, getFogFactor(fFogCoord));
	switch(DrawMode) {
		case 0: frag_COLOR = vec4(result, transparency);								break;
		case 1: frag_COLOR = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);		break;
		case 2: frag_COLOR = specMap;													break;
		case 3: frag_COLOR = vec4(fs_in.frag_NORMAL * 0.5 + 0.5,1.0);					break;
		case 4: frag_COLOR = norm;														break;
		case 5: frag_COLOR = normMap;													break;
		case 6: frag_COLOR = vec4(fs_in.frag_TAN * 0.5 + 0.5,1.0);						break;
		case 7: frag_COLOR = vec4(fs_in.frag_BITAN * 0.5 + 0.5,1.0);					break;
		case 8: frag_COLOR = emisMap;													break;
		case 9: frag_COLOR = vec4(skyboxCalc,1.0);										break;
	}
	float brightness = dot(frag_COLOR.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0) frag_BRIGHTNESS = vec4(frag_COLOR.rgb, 1.0);
    else frag_BRIGHTNESS = vec4(0.0, 0.0, 0.0, 1.0);
}

vec3 calcDirLight(vec4 diffMap, vec4 specMap, vec4 emisMap, vec4 normMap) {
	// ambient
    vec3 ambient = dirLight.ambient * diffMap.rgb;
  	
    // diffuse 
    vec3 norm = normMap.rgb;
    vec3 lightDir = normalize(-dirLight.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLight.diffuse * diffMap.rgb;
    
    // specular
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
	float spec = 0;
	vec3 reflectDir = reflect(-lightDir, norm);
	spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	if(lightingType==3) spec += pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    if(lightingType==2) spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = specMap.rgb * spec * dirLight.specular;
	
	return ambient + diffuse + specular;
}

vec3 calcPointLight(int pLightIndex, vec4 diffMap, vec4 specMap, vec4 emisMap, vec4 normMap) {
	// ambient
    vec3 ambient = pointLights[pLightIndex].ambient * diffMap.rgb;
  	
    // diffuse
	TangentLightPos = fs_in.tan_MATRIX * pointLights[pLightIndex].position;
    vec3 norm = normMap.rgb;
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLight.diffuse * diffMap.rgb;
    
    // specular
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
	float spec = 0;
	vec3 reflectDir = reflect(-lightDir, norm);
	spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	if(lightingType==3) spec += pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    if(lightingType==2) spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = specMap.rgb * spec * pointLights[pLightIndex].specular;
	
	// attenuation
	float attDistance = length(pointLights[pLightIndex].position - fs_in.frag_POS);
	float attenuation = 1.0 / attDistance;
	
	return (ambient + diffuse + specular) * attenuation;
}

vec3 calcSpotLight(int sLightIndex, vec4 diffMap, vec4 specMap, vec4 emisMap, vec4 normMap) {
	// ambient
	vec3 ambient = spotLights[sLightIndex].ambient * diffMap.rgb;
	
	// diffuse
	TangentLightPos = fs_in.tan_MATRIX * spotLights[sLightIndex].position;
    vec3 norm = normMap.rgb;
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLight.diffuse * diffMap.rgb;
    
    // specular
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
	float spec = 0;
	vec3 reflectDir = reflect(-lightDir, norm);
	spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	if(lightingType==3) spec += pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    if(lightingType==2) spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
	vec3 specular = specMap.rgb * spec * spotLights[sLightIndex].specular;
	
	// spotLights[sLightIndex] (soft edges)
    float theta = dot(lightDir, normalize(-spotLights[sLightIndex].direction)); 
    float epsilon = (spotLights[sLightIndex].cutOff - spotLights[sLightIndex].outerCutOff);
    float intensity = clamp((theta - spotLights[sLightIndex].outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;
	
	// attenuation
	float attDistance = length(spotLights[sLightIndex].position - fs_in.frag_POS);
	float attenuation = 1.0 / attDistance;
	
	return (ambient + diffuse + specular) * attenuation;
}