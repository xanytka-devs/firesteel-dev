#version 330 core

in vec2 frag_UV;
out vec4 frag_COLOR;

uniform sampler2D texture_diffuse1;
uniform int DrawMode;

float near = 0.1; 
float far  = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
    vec4 texColor = texture(texture_diffuse1, frag_UV);
	if(texColor.a<0.5) discard;
	
	if(DrawMode==0) frag_COLOR = texColor;
	else if(DrawMode==1) frag_COLOR = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
}