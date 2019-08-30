#ifndef FONT_H
#define FONT_H
struct font_context {
	int w;
	int h;
	int ascent;
	void * font;
	void * dc;
//    int edge;
};

void font_size(const char *str, int unicode, struct font_context * ctx);
void font_glyph(const char * str, int unicode, void * buffer, struct font_context * ctx);
void font_create(int font_size, struct font_context *ctx);
void font_release(struct font_context *ctx);


#endif
