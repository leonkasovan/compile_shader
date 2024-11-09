gl:
	gcc main.c -o compile_shader -lGL `sdl2-config --cflags --libs`

gles:
	gcc -DGLES main.c -o compile_shader -lGLESv2 `sdl2-config --cflags --libs`
