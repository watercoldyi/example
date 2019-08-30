all:font.dll

font.dll: dfont.c winfont.c lua-font.c
	gcc -Wall --shared -o $@ $^ -lgdi32 -llua
