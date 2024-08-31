#version 330 core

out vec4 frag_COLOR;

uniform vec4 color;

uniform int DrawMode;
float near = 0.1; 
float far  = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
    if(DrawMode==0) frag_COLOR = color;
	else if(DrawMode==1) frag_COLOR = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
}
