#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 6) out;

uniform vec3 up;
uniform vec3 right;

in float size [];

void main () {
    vec3	u = up * size [0];
    vec3	r = right * size [0];
    vec3	p = gl_in [0].gl_Position.xyz;
    float	w = gl_in [0].gl_Position.w;
	
    gl_Position = vec4 ( p - u - r, w );
    EmitVertex ();
	
    gl_Position = vec4 ( p - u + r, w );
    EmitVertex ();
	
    gl_Position = vec4 ( p + u + r, w );
    EmitVertex   ();
    EndPrimitive ();				// 1st triangle
	
    gl_Position = vec4 ( p + u + r, w );
    EmitVertex   ();
	
    gl_Position = vec4 ( p + u - r, w );
    EmitVertex ();
	
    gl_Position = vec4 ( p - u - r, w );
    EmitVertex   ();
    EndPrimitive ();				// 2nd triangle
}