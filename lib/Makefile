all: window.dll gl.dll

window.dll: lua-window.c
	gcc --shared -o $@ $^ -luser32 -lgdi32 -llua

gl.dll: lua-gl.c ppm.c
	gcc --shared -o $@ $^ -lgdi32 -lglew32 -lopengl32 -llua

install: gl.dll window.dll
	cp $^ /mingw64/lib/lua/5.3/
