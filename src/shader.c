#include <GL/glew.h>
#include <shader.h>

#include <stdio.h>
#include <string.h>

GLuint load_shader (char* vp, char* fp){
	FILE* v_file = fopen (vp, "r");
	FILE* f_file = fopen (fp, "r");

	if( v_file == NULL || f_file == NULL){
		printf("Couldn't open files %s or %s \n", vp, fp);
		return 0;
	}

	char vertex_buf [MAX_SHADER_CHARACTERS];
	char fragmt_buf [MAX_SHADER_CHARACTERS];

	fseek(v_file, 0, SEEK_END);
	uint32_t v_length = ftell (v_file);
	fseek(v_file, 0, SEEK_SET);
	fread (vertex_buf, 1, v_length, v_file);

	fseek(f_file, 0, SEEK_END);
	uint32_t f_length = ftell (f_file);
	fseek(f_file, 0, SEEK_SET);
	fread (fragmt_buf, 1, f_length, f_file);

	vertex_buf [v_length] = '\0';
	fragmt_buf [f_length] = '\0';

	GLuint sp = glCreateProgram();

	const char* vert_c = vertex_buf;
	const char* frag_c = fragmt_buf;

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmt_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertex_shader, 1, (&vert_c), NULL);
	glShaderSource(fragmt_shader, 1, (&frag_c), NULL);
	glCompileShader(vertex_shader);
	glCompileShader(fragmt_shader);

	GLint compile_status;
	glGetShaderiv (vertex_shader, GL_COMPILE_STATUS, &compile_status);

	if(compile_status == GL_FALSE){
		char statuslog [512];
		int t;
		glGetShaderInfoLog (vertex_shader, 512, &t, statuslog);
		printf("Vertex Shader Compilation of %s failed: \n %s \n", vp, statuslog);
	}

	glGetShaderiv (fragmt_shader, GL_COMPILE_STATUS, &compile_status);

	if(compile_status == GL_FALSE){
		char statuslog [512];
		int t;
		glGetShaderInfoLog (fragmt_shader, 512, &t, statuslog);
		printf("Fragment Shader Compilation of %s failed: \n %s \n", fp, statuslog);
	}

	glAttachShader (sp, vertex_shader);
	glAttachShader (sp, fragmt_shader);
	glLinkProgram  (sp);

	glGetProgramiv (sp, GL_LINK_STATUS, &compile_status);

	if(compile_status == GL_FALSE){
		char statuslog [512];
		int t;
		glGetProgramInfoLog (sp, 512, &t, statuslog);
		printf("Shader Linking of %s %s failed: \n %s \n", vp, fp, statuslog);
	}

	fclose(v_file);
	fclose(f_file);

	return sp;
}
