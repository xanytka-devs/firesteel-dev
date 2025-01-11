#version 330 core
layout (location = 0) out vec4 frag_COLOR;
layout (location = 1) out vec4 frag_BRIGHTNESS;

in vec2 frag_UV;

uniform sampler2D diffuse0;
uniform int drawMode;
uniform vec3 color;

float near = 0.1; 
float far  = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
    vec4 texColor = texture(diffuse0, frag_UV);
	if(texColor.a<0.5) discard;
	
	if(drawMode==0) frag_COLOR = texColor * vec4(color, 1.0);
	else if(drawMode==1) frag_COLOR = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
	frag_BRIGHTNESS = vec4(vec3(0), 1);
}