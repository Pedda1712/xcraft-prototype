#version 110 

attribute vec3 apos;
attribute vec2 atex;
attribute float acol;

varying vec4 color;
varying vec2 tex;

void main () {
	color = vec4(acol, acol, acol, 1.0);
	tex = atex;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(apos,1.0);
}
