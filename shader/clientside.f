#version 110

varying vec4 color;
varying vec2 tex;

uniform sampler2D obj;

void main () {
	vec4 texc = texture2D(obj, tex);
	gl_FragColor =  texc * color;
	
	if(texc.a == 0.0){
		discard;
	}
}
