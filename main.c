/*
Command line tool to test shader compiler (using OpenGL + SDL2).
Run it inside folder that contains shader files.
If filename consists "frag" then it treat as fragment shader.
If filename consists "vert" then it treat as vertex shader.
If filename consists "geo" then it treat as geometry shader.

Build:
gcc main.c -o compile_shader -lGL `sdl2-config --cflags --libs`
gcc main.c -o compile_shader -lGLESv2 `sdl2-config --cflags --libs`
*/

#ifdef GLES
#include <GLES/gl.h>
#define HEADER_SCRIPT "#version 300 es\nprecision highp float;\n"
#else
#include <GL/gl.h>
// #define HEADER_SCRIPT "#version 120\n"
#define HEADER_SCRIPT "#version 330\n"
#endif

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_GEOMETRY_SHADER_EXT 0x8DD9
#define GL_COMPILE_STATUS 0x8B81
#define GL_INFO_LOG_LENGTH 0x8B84

void glCompileShader(GLuint shader);
GLuint glCreateShader(GLenum shaderType);
void glDeleteShader(GLuint shader);
void glGetShaderInfoLog(GLuint shader,
						GLsizei maxLength,
						GLsizei *length,
						GLchar *infoLog);
void glShaderSource(GLuint shader,
					GLsizei count,
					const GLchar *const *string,
					const GLint *length);
void glGetShaderiv(GLuint shader,
				   GLenum pname,
				   GLint *params);
int chdir(const char *path);

// Function to read shader source from a file
char *readShaderSource(const char *shaderFile)
{
	long header_length = strlen(HEADER_SCRIPT);
	FILE *file = fopen(shaderFile, "rb");
	if (!file)
	{
		printf("Error: Could not open shader file %s\n", shaderFile);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *source = (char *)malloc(header_length+length + 1);
	strcpy(source, HEADER_SCRIPT);
	fread(source + header_length, 1, length, file);
	source[length + header_length] = '\0';
	fclose(file);
	return source;
}

// Function to compile a shader
GLuint compileShader(const char *shaderFile, GLenum shaderType)
{
	// Read the shader source from file
	char *shaderSource = readShaderSource(shaderFile);
	if (!shaderSource)
	{
		return 0;
	}
	// Create and compile the shader
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const char **)&shaderSource, NULL);
	glCompileShader(shader);

	// Check for compilation errors
	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	printf("\n--------------------------------------------------------------------\nCompiling %s ...\n",
		   shaderFile);
	if (compileStatus == GL_FALSE)
	{
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		char *infoLog = (char *)malloc(logLength);
		glGetShaderInfoLog(shader, logLength, NULL, infoLog);
		// printf("Error compiling shader (%s): %s\n", shaderFile, infoLog);
		printf("%s", infoLog);
		free(infoLog);
		glDeleteShader(shader);
		free(shaderSource);
		return 0;
	}
	// Clean up and return the shader ID
	free(shaderSource);
	return shader;
}

void printOpenGLInfo()
{
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);

	if (vendor && renderer && version)
	{
#ifdef GLES
		printf("OpenGL ES Vendor   : %s\n", vendor);
		printf("OpenGL ES Renderer : %s\n", renderer);
		printf("OpenGL ES Version  : %s\n", version);
#else
		printf("OpenGL Vendor   : %s\n", vendor);
		printf("OpenGL Renderer : %s\n", renderer);
		printf("OpenGL Version  : %s\n", version);
#endif
	}
	else
	{
		printf("Failed to retrieve OpenGL information.\n");
	}
}

int main(int argc, char *argv[])
{
	// Open the directory specified by the user
	const char *user_dir = argc >= 2 ? argv[1] : ".";
	printf("Tool for shader testing compilation in directory '%s'\n", user_dir);

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
		return -1;
	}
#ifdef GLES
	// Set SDL to use an OpenGLES context (version 3.0)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_ES);
#else
	// Set SDL to use an OpenGL context (version 2.1 for compatibility with OpenGL 2.1)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
	// 					SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	// Create a window with an OpenGL context
	SDL_Window *window = SDL_CreateWindow("Shader Compile Test",
										  SDL_WINDOWPOS_CENTERED,
										  SDL_WINDOWPOS_CENTERED,
										  640, 480,
										  SDL_WINDOW_OPENGL |
											  SDL_WINDOW_SHOWN);

	if (!window)
	{
		fprintf(stderr, "Failed to create SDL window: %s\n",
				SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!glContext)
	{
		fprintf(stderr, "Failed to create OpenGL context: %s\n",
				SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	printOpenGLInfo();

	chdir(user_dir);
	DIR *dir = opendir(user_dir);
	if (dir == NULL)
	{
		perror("Unable to open directory");
		return 1;
	}

	struct dirent *entry;
	// Loop through directory entries
	while ((entry = readdir(dir)) != NULL)
	{
		// Check if 'frag' is in the filename
		if (strstr(entry->d_name, "frag") != NULL)
		{
			printf("Result %s %s\n", entry->d_name,
				   compileShader(entry->d_name,
								 GL_FRAGMENT_SHADER)
					   ? "OK"
					   : "FAIL");
		}
		else if (strstr(entry->d_name, "vert") != NULL)
		{
			printf("Result %s %s\n", entry->d_name,
				   compileShader(entry->d_name,
								 GL_VERTEX_SHADER)
					   ? "OK"
					   : "FAIL");
		}
		else if (strstr(entry->d_name, "geo") != NULL)
		{
			printf("Result %s %s\n", entry->d_name,
				   compileShader(entry->d_name,
								 GL_GEOMETRY_SHADER_EXT)
					   ? "OK"
					   : "FAIL");
		}
	}
	// Close the directory
	closedir(dir);
	return 0;
}
