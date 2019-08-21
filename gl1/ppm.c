#include "render.h"

#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PPM_RGBA8 0
#define PPM_RGB8 1
#define PPM_RGBA4 2
#define PPM_RGB4 3
#define PPM_ALPHA8 4
#define PPM_ALPHA4 5

#define ARRAY(type, name, size) type name[size]

struct ppm {
	int type;
	int depth;
	int step;
	int width;
	int height;
	uint8_t *buffer;
};

#define LINEMAX 128

static char *
readline(FILE *f, char *buffer) {
	for (;;) {
		char * ret = fgets(buffer, LINEMAX, f);
		if (ret == NULL) {
			return NULL;
		}
		if (ret[0] != '#') {
			return ret;
		}
	}
}

static int
ppm_header(FILE *f, struct ppm *ppm) {
	char tmp[LINEMAX];
	char *line = readline(f, tmp);
	if (line == NULL)
		return 0;
	char c = 0;
	sscanf(line, "P%c", &c);
	ppm->type = c;
	line = readline(f, tmp);
	if (line == NULL)
		return 0;
	sscanf(line, "%d %d", &(ppm->width), &(ppm->height));
	line = readline(f, tmp);
	if (line == NULL)
		return 0;
	sscanf(line, "%d", &(ppm->depth));
	return 1;
}

static int
ppm_data(struct ppm *ppm, FILE *f, int id, int skip) {
	int i;
	int n = ppm->width * ppm->height;
	uint8_t * buffer = ppm->buffer + skip;
	uint8_t * tmp;
	int step = ppm->step;
	switch(id) {
	case '3':	// RGB text
		for (i=0;i<n;i++) {
			int r,g,b;
			fscanf(f, "%d %d %d", &r,&g,&b);
			buffer[i*step+0] = (uint8_t)r;
			buffer[i*step+1] = (uint8_t)g;
			buffer[i*step+2] = (uint8_t)b;
		}
		break;
	case '2':	// ALPHA text
		for (i=0;i<n;i++) {
			int alpha;
			fscanf(f, "%d", &alpha);
			buffer[i*step] = (uint8_t)alpha;
		}
		break;
	case '6':	// RGB binary
		tmp = (uint8_t *)malloc(n * 3);
		if (fread(tmp, n*3, 1, f)==0) {
			free(tmp);
			return 0;
		}
		for (i=0;i<n;i++) {
			buffer[i*step+0] = tmp[i*3+0];
			buffer[i*step+1] = tmp[i*3+1];
			buffer[i*step+2] = tmp[i*3+2];
		}
		free(tmp);
		break;
	case '5':	// ALPHA binary
		tmp = (uint8_t *)malloc(n);
		if (fread(tmp, n, 1, f)==0) {
			free(tmp);
			return 0;
		}
		for (i=0;i<n;i++) {
			buffer[i*step] = tmp[i];
		}
		free(tmp);
		break;
	default:
		return 0;
	}
	return 1;
}

static int
loadppm_from_file(FILE *rgb, FILE *alpha, struct ppm *ppm) {
	ppm->buffer = NULL;
	ppm->step = 0;
	int rgb_id = 0;
	int alpha_id = 0;
	if (rgb) {
		if (!ppm_header(rgb, ppm)) {
			return 0;
		}
		rgb_id = ppm->type;
		ppm->step += 3;
	}
	if (alpha) {
		if (rgb == NULL) {
			if (!ppm_header(alpha, ppm)) {
				return 0;
			}
			alpha_id = ppm->type;
		} else {
			struct ppm pgm;
			if (!ppm_header(alpha, &pgm)) {
				return 0;
			}
			if (ppm->depth != pgm.depth || ppm->width != pgm.width || ppm->height != pgm.height) {
				return 0;
			}
			alpha_id = pgm.type;
		}
		ppm->step += 1;
	}
	ppm->buffer = (uint8_t *)malloc(ppm->height * ppm->width * ppm->step);
	if (rgb) {
		if (!ppm_data(ppm, rgb, rgb_id, 0))
			return 0;
	}
	if (alpha) {
		int skip = 0;
		if (rgb) {
			skip = 3;
		}
		if (!ppm_data(ppm, alpha, alpha_id, skip)) 
			return 0;
	}

	return 1;
}

static int
loadtexture(lua_State *L) {
	size_t sz = 0;
	const char * filename = luaL_checklstring(L, 1, &sz);
	ARRAY(char, tmp, sz + 5);
	sprintf(tmp, "%s.ppm", filename);
	FILE *rgb = fopen(tmp, "rb");
	sprintf(tmp, "%s.pgm", filename);
	FILE *alpha = fopen(tmp, "rb");
	if (rgb == NULL && alpha == NULL) {
		return luaL_error(L, "Can't open %s(.ppm/.pgm)", filename);
	}
	struct ppm ppm;

	int ok = loadppm_from_file(rgb, alpha, &ppm);

	if (rgb) {
		fclose(rgb);
	}
	if (alpha) {
		fclose(alpha);
	}
	if (!ok) {
		if (ppm.buffer) {
			free(ppm.buffer);
		}
		luaL_error(L, "Invalid file %s", filename);
	}

	int type = 0;
	if (ppm.depth == 255) {
		if (ppm.step == 4) {
			type = TEX_RGBA8;
		} else if (ppm.step == 3) {
			type = TEX_RGB;
		} else {
			type = TEX_A8;
		}
	} else {
		if (ppm.step == 4) {
			type = TEX_RGBA8;
			uint16_t * tmp = (uint16_t * )malloc(ppm.width * ppm.height * sizeof(uint16_t));
			int i;
			for (i=0;i<ppm.width * ppm.height;i++) {
				uint32_t r = ppm.buffer[i*4+0];
				uint32_t g = ppm.buffer[i*4+1];
				uint32_t b = ppm.buffer[i*4+2];
				uint32_t a = ppm.buffer[i*4+3];
				tmp[i] = r << 12 | g << 8 | b << 4 | a;
			}
			free(ppm.buffer);
			ppm.buffer = (uint8_t*)tmp;
		} else if (ppm.step == 3) {
			type = TEX_RGB565;
			uint16_t * tmp = (uint16_t *)malloc(ppm.width * ppm.height * sizeof(uint16_t));
			int i;
			for (i=0;i<ppm.width * ppm.height;i++) {
				uint32_t r = ppm.buffer[i*3+0];
				if (r == 15) {
					r = 31;
				} else {
					r <<= 1;
				}
				uint32_t g = ppm.buffer[i*3+1];
				if (g == 15) {
					g = 63;
				} else {
					g <<= 2;
				}
				uint32_t b = ppm.buffer[i*3+2];
				if (b == 15) {
					b = 31;
				} else {
					b <<= 1;
				}
				tmp[i] = r << 11 | g << 5 | b;
			}
			free(ppm.buffer);
			ppm.buffer = (uint8_t*)tmp;
		} else {
			type = TEX_RGBA8;
			int i;
			for (i=0;i<ppm.width * ppm.height;i++) {
				uint8_t c =	ppm.buffer[i];
				if (c == 15) {
					ppm.buffer[i] = 255;
				} else {
					ppm.buffer[i] *= 16;
				}
			}
		}
	}
    struct texture *tex = malloc(sizeof(*tex));
    tex->fmt = type;
    tex->w = ppm.width;
    tex->h = ppm.height;
    tex->data = ppm.buffer;
    lua_pushlightuserdata(L,tex);
	return 1;
}


int 
luaopen_glu_ppm(lua_State *L) {
	luaL_Reg l[] = {
		{ "texture", loadtexture },
		{ NULL, NULL },
	};

	luaL_newlib(L,l);

	return 1;
}
