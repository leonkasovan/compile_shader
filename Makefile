gl:
	$(CC) main.c -o compile_shader -lGL `sdl2-config --cflags --libs`

gles:
	$(CC) -DGLES main.c -o compile_shader -lGLESv2 `sdl2-config --cflags --libs`

rg353p:
	$(CC) -DRG353P main.c -o compile_shader -lmali `sdl2-config --libs`

pi:
	$(CC) -DGLES main.c -o compile_shader -lGLESv2 `sdl2-config --cflags --libs`	