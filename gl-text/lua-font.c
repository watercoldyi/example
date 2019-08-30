#include "font.h"
#include "dfont.h"
#include <lua.h>
#include <lauxlib.h>

#define DFONT_NAME "dfont"
#define FONT_NAME "font"

struct font_ud {
	struct dfont *font;
};

static int
ldfont_release(lua_State *L){
	struct font_ud *ud = lua_touserdata(L,1);
	dfont_release(ud->font);
	ud->font = NULL;
	return 0;
}

static int
ldfont_lookup(lua_State *L){
	struct font_ud *ud = luaL_checkudata(L,1,DFONT_NAME);
	int c = luaL_checkinteger(L,2);
	int font = luaL_checkinteger(L,3);
	int edge = luaL_checkinteger(L,4);
	const struct dfont_rect *rect = dfont_lookup(ud->font,c,font,edge);
	if(rect == NULL){return 0;}
	else{
		lua_pushinteger(L,rect->x);
		lua_pushinteger(L,rect->y);
		lua_pushinteger(L,rect->w);
		lua_pushinteger(L,rect->h);
		return 4;
	}
}

static int
ldfont_insert(lua_State *L){
	struct font_ud *ud = luaL_checkudata(L,1,DFONT_NAME);
	int c = luaL_checkinteger(L,2);
	int font = luaL_checkinteger(L,3);
	int w = luaL_checkinteger(L,4);
	int h = luaL_checkinteger(L,5);
	int edge = luaL_checkinteger(L,6);
	const struct dfont_rect *rect = dfont_insert(ud->font,c,font,w,h,edge);
	if(rect == NULL){return 0;}
	else{
		lua_pushinteger(L,rect->x);
		lua_pushinteger(L,rect->y);
		lua_pushinteger(L,rect->w);
		lua_pushinteger(L,rect->h);
		return 4;
	}
}

static int
ldfont_flush(lua_State *L){
	struct font_ud *ud = luaL_checkudata(L,1,DFONT_NAME);
	dfont_flush(ud->font);
	return 0;
}

static int
ldfont_dump(lua_State *L){
	struct font_ud *ud = luaL_checkudata(L,1,DFONT_NAME);
	dfont_dump(ud->font);
	return 0;
}

static int
ldfont_create(lua_State *L){
	int w = luaL_checkinteger(L,1);
	int h = luaL_checkinteger(L,2);
	struct font_ud *ud = lua_newuserdata(L,sizeof(*ud));
	ud->font = dfont_create(w,h);
	static luaL_Reg f[] = {
		{"lookup",ldfont_lookup},
		{"insert",ldfont_insert},
		{"flush",ldfont_flush},
		{"dump",ldfont_dump},
		{"__gc",ldfont_release},
		{NULL,NULL}
	};
	if(luaL_newmetatable(L,DFONT_NAME)){
		luaL_newlib(L,f);
		lua_setfield(L,-2,"__index");
	}
	lua_setmetatable(L,-2);
	return 1;
}

static int
lfont_release(lua_State *L){
	struct font_context *ud = lua_touserdata(L,1);
	font_release(ud);
	return 0;
}

static int
lfont_size(lua_State *L){
	struct font_context *ud = luaL_checkudata(L,1,FONT_NAME);
	int c = luaL_checkinteger(L,2);
	font_size(NULL,c,ud);
	lua_pushinteger(L,ud->w);
	lua_pushinteger(L,ud->h);
	return 2;
}

static int
lfont_glyph(lua_State *L){
	char buf[255*255]={0};
	struct font_context *ud = luaL_checkudata(L,1,FONT_NAME);
	int c = luaL_checkinteger(L,2);
	font_glyph(NULL,c,buf,ud);
	int size = ud->w * ud->h;
	lua_pushlstring(L,buf,size);
	return 1;
}

static int
lfont_create(lua_State *L){
	int size = luaL_checkinteger(L,1);
	struct font_context *ud = lua_newuserdata(L,sizeof(*ud));
	font_create(size,ud);
	static luaL_Reg f[] = {
		{"size",lfont_size},
		{"glyph",lfont_glyph},
		{"__gc",lfont_release},
		{NULL,NULL}
	};
	if(luaL_newmetatable(L,FONT_NAME)){
		luaL_newlib(L,f);
		lua_setfield(L,-2,"__index");
	}
	lua_setmetatable(L,-2);
	return 1;
}

int
luaopen_font(lua_State *L){
	static luaL_Reg f[] = {
		{"dfont_create",ldfont_create},
		{"font_create",lfont_create},
		{NULL,NULL}
	};
	luaL_newlib(L,f);
	return 1;
}
