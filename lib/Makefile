
TARGET = window.dll gl.dll stbi.dll 
all: $(TARGET) 

window.dll: lua-window.c
	gcc --shared -o $@ $^ -luser32 -lgdi32 -llua

gl.dll: lua-gl.c ppm.c
	gcc --shared -o $@ $^ -lgdi32 -lglew32 -lopengl32 -llua

stbi.dll: lua-stb-image.c
	gcc --shared -o $@ $^ -llua 

install: $(TARGET) 
	cp $^ /mingw64/lib/lua/5.3/
