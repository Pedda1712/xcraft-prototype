#version 110

varying vec4 color;
varying vec2 tex;

uniform sampler2D obj;

void main () {
	gl_FragColor = texture2D(obj, tex) * color;
}
