#include <lua.h>
#include <lauxlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static int
lload(lua_State *L){
	const char *path = luaL_checkstring(L,1);
	int desired = luaL_checkinteger(L,2);
	int w,h,n;
	unsigned char * data = stbi_load(path,&w,&h,&n,desired);
	if(data){
		lua_pushlightuserdata(L,data);
		lua_pushinteger(L,w);
		lua_pushinteger(L,h);
		lua_pushinteger(L,n);
		return 4;
	} else{ return 0;} 
}

static int
lfree(lua_State *L){
	unsigned char *data = lua_touserdata(L,1);
	if(data){stbi_image_free(data);}
	return 0;
}

int
luaopen_stbi(lua_State *L){
	static luaL_Reg f[] = {
		{"load",lload},
		{"free",lfree},
		{NULL,NULL}	
	};
	luaL_newlib(L,f);
	return 1;
}
