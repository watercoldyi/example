all: window.dll glu.dll

window.dll: lua-window.c
	gcc --shared -o $@ $^ -luser32 -lgdi32 -llua

glu.dll: lua-gl.c ppm.c
	gcc --shared -o $@ $^ -lgdi32 -lglew32 -lopengl32 -llua
