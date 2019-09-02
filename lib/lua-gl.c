#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include "render.h"

static void
_check_gl_error(lua_State *L){
    GLenum code = glGetError();
    if(code != GL_NO_ERROR){
        luaL_error(L,"gl error %d!",code);
    }
}

#define CHECK_GL_ERROR(L) _check_gl_error(L);

static struct render *_rs;

static void
_set_pixel_format_to_hdc(HDC hDC){
    int color_deep;
    PIXELFORMATDESCRIPTOR pfd;

    color_deep = GetDeviceCaps(hDC, BITSPIXEL);

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = color_deep;
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);

    SetPixelFormat(hDC, pixelFormat, &pfd);
}

static int
linit(lua_State *L){
    HDC dc = (HDC)luaL_checkinteger(L,1);
    _set_pixel_format_to_hdc(dc);
    HGLRC hglrc = wglCreateContext(dc);
    wglMakeCurrent(dc,hglrc);
    if(glewInit() != GLEW_OK){
        luaL_error(L,"init gl fail");
    }
    return 0;
}

static int
lviewport(lua_State *L){
    int w = luaL_checkinteger(L,1);
    int h = luaL_checkinteger(L,2);
    glViewport(0,0,w,h);
    return 0;
}

static int
lnew_program(lua_State *L){
    GLuint program = glCreateProgram();
    lua_pushinteger(L,program);
    return 1;
}

static int
lglLinkProgram(lua_State *L){
    GLuint program = luaL_checkinteger(L,1);
    glLinkProgram(program);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglBufferData(lua_State *L){
    GLenum type = luaL_checkinteger(L,1);
    luaL_checktype(L,2,LUA_TTABLE);
    size_t len = lua_rawlen(L,2);
    GLenum opt = luaL_checkinteger(L,3);
    float *buf = malloc(len * sizeof(float));
    size_t i;
    for(i = 1;i <= len;i++){
        lua_rawgeti(L,2,i);
        buf[i-1] = (float)lua_tonumber(L,-1);
        lua_pop(L,1);
    }
    glBufferData(type,len * sizeof(float),buf,opt);
    free(buf);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglBufferDatai2(lua_State *L){
    GLenum type = luaL_checkinteger(L,1);
    luaL_checktype(L,2,LUA_TTABLE);
    size_t len = lua_rawlen(L,2);
    GLenum opt = luaL_checkinteger(L,3);
    uint16_t *buf = malloc(len * sizeof(uint16_t));
    size_t i;
    for(i = 1;i <= len;i++){
        lua_rawgeti(L,2,i);
        buf[i-1] = (uint16_t)lua_tointeger(L,-1);
        lua_pop(L,1);
    }
    glBufferData(type,len * sizeof(uint16_t),buf,opt);
    free(buf);
    CHECK_GL_ERROR(L)
    return 0;   
}



static int
lclear(lua_State *L){
    int color = luaL_checkinteger(L,1);
    float r = (float)(color >> 24) / 255;
    float g = (float)((color & 0x00ff0000) >> 16) / 255;
    float b = (float)((color & 0xff00) >> 8) / 255;
	float a = (color & 0xff) / 255;
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

static int
lglUseProgram(lua_State *L){
    GLuint p = luaL_checkinteger(L,1);
    glUseProgram(p);
    return 0;
}

int
lvertexattr(lua_State *L){
    int id = luaL_checkinteger(L,1);
    int size = luaL_checkinteger(L,2);
    GLenum type = luaL_checkinteger(L,3);
    GLenum normalize = luaL_checkinteger(L,4);
    GLuint strip = luaL_checkinteger(L,5);
    GLuint pointer = luaL_checkinteger(L,6);
    glVertexAttribPointer(id,size,type,normalize,strip,(const void *)pointer);
    glEnableVertexAttribArray(id);
    return 0;
}

static void
_set_constant(lua_State *L,const char *name,int v){
    lua_pushinteger(L,v);
    lua_setfield(L,-2,name);
}

static int
lupdate_texture(lua_State *L){
    struct texture *tex = lua_touserdata(L,1);
    GLuint id = luaL_checkinteger(L,2);
    GLenum glfmt = 0;
    GLenum type = 0;
    switch(tex->fmt){
        case TEX_RGBA8:
            glfmt = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case TEX_RGBA4:
            glfmt = GL_RGBA;
            type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case TEX_RGB:
            glfmt = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            break;
        case TEX_RGB565:
            glfmt = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            break;
    }
    glTexImage2D(GL_TEXTURE_2D,0,glfmt,tex->w,tex->h,0,glfmt,type,tex->data);
    free(tex);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglGenTextures(lua_State *L){
    GLuint id;
    glGenTextures(1,&id);
    lua_pushinteger(L,id);
    return 1;
}

static int
lglBindTexture(lua_State *L){
    GLuint id = luaL_checkinteger(L,1);
    glBindTexture(GL_TEXTURE_2D,id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglDrawElements(lua_State *L){
    GLenum mode = luaL_checkinteger(L,1);
    int n = luaL_checkinteger(L,2);
    GLenum type = luaL_checkinteger(L,3);
    uint64_t offset = luaL_checkinteger(L,4);
    glDrawElements(mode,n,type,(const void *)offset);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglDrawArrays(lua_State *L){
    GLenum mode = luaL_checkinteger(L,1);
    int first = luaL_checkinteger(L,2);
    int n = luaL_checkinteger(L,3);
    glDrawArrays(mode,first,n);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lSwapBuffer(lua_State *L){
    HDC dc = (HDC)luaL_checkinteger(L,1);
    SwapBuffers(dc);
    return 0;
}

static int
lglCreateProgram(lua_State *L){
    int id = glCreateProgram();
    lua_pushinteger(L,id);
    return 1;
}

static int
lglCreateShader(lua_State *L){
    GLenum type = luaL_checkinteger(L,1);
    int id = glCreateShader(type);
    lua_pushinteger(L,id);
    return 1;
}

static int
lglShaderSource(lua_State *L){
    GLint sd = luaL_checkinteger(L,1);
    const char *glsl = luaL_checkstring(L,2);
    const GLchar *source[] = {glsl};
    glShaderSource(sd,1,source,NULL);
    return 0;
}

static int
lglCompileShader(lua_State *L){
    GLint sd = luaL_checkinteger(L,1);
    glCompileShader(sd);
    GLint ret = GL_TRUE;
    glGetShaderiv(sd,GL_COMPILE_STATUS,&ret);
    char buf[1024];
    GLint len;
    if(ret == GL_FALSE){
        glGetShaderInfoLog(sd,1024,(GLint*)&len,buf);
        luaL_error(L,"Compile Shader fail error log: %s \nshader code:\n%s\n",buf,ret);
    }
    return 0;
}

static int
lglAttachShader(lua_State *L){
    GLint sd = luaL_checkinteger(L,1);
    GLint program = luaL_checkinteger(L,2);
    glAttachShader(program,sd);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglGenVertexArrays(lua_State *L){
    GLint vao;
    glGenVertexArrays(1,&vao);
    CHECK_GL_ERROR(L)
    lua_pushinteger(L,vao);
    return 1;
}

static int
lglBindVertexArray(lua_State *L){
    GLint vao = luaL_checkinteger(L,1);
    glBindVertexArray(vao);
    CHECK_GL_ERROR(L)
}

static int
lglGenBuffers(lua_State *L){
    GLint id;
    glGenBuffers(1,&id);
    lua_pushinteger(L,id);
    return 1;
}

static int
lglBindBuffer(lua_State *L){
    GLenum target = luaL_checkinteger(L,1);
    GLint id = luaL_checkinteger(L,2);
    glBindBuffer(target,id);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglTexParameteri(lua_State *L){
    GLenum target = luaL_checkinteger(L,1);
    GLenum name = luaL_checkinteger(L,2);
    GLenum p = luaL_checkinteger(L,3);
    glTexParameteri(target,name,p);
    CHECK_GL_ERROR(L)
    return 0;
}

static int
lglUniformMatrix4fv(lua_State *L){
    GLint location = luaL_checkinteger(L,1);
    GLboolean trans = lua_toboolean(L,2);
    luaL_checktype(L,3,LUA_TTABLE);
    size_t len = lua_rawlen(L,3);
    float *buf = malloc(len * sizeof(float));
    size_t i;
    for(i = 1;i <= len;i++){
        lua_rawgeti(L,3,i);
        buf[i-1] = (float)lua_tonumber(L,-1);
        lua_pop(L,1);
    }
    glUniformMatrix4fv(location,1,trans,buf);
    free(buf);
    return 0;

}

static int
lTexImage2D(lua_State *L){
	GLenum target = luaL_checkinteger(L,1);
	GLint level = luaL_checkinteger(L,2);
	GLenum lfmt = luaL_checkinteger(L,3);
	GLint w = luaL_checkinteger(L,4);
	GLint h = luaL_checkinteger(L,5);
	GLint border = luaL_checkinteger(L,6);
	GLenum fmt = luaL_checkinteger(L,7);
	GLenum type = luaL_checkinteger(L,8);
	const char *data = NULL;
	int dtype = lua_type(L,9);
	if(dtype == LUA_TSTRING){
		data = lua_tostring(L,9);
	} else if(dtype == LUA_TLIGHTUSERDATA){
		data = lua_touserdata(L,9);	
	}
	glTexImage2D(target,level,lfmt,w,h,border,fmt,type,data);
	CHECK_GL_ERROR(L)
	return 0;
}

static int
lTexSubImage2D(lua_State *L){
	GLenum target = luaL_checkinteger(L,1);
	GLint level = luaL_checkinteger(L,2);
	GLint x = luaL_checkinteger(L,3);
	GLint y = luaL_checkinteger(L,4);
	GLint w = luaL_checkinteger(L,5);
	GLint h = luaL_checkinteger(L,6);
	GLenum fmt = luaL_checkinteger(L,7);
	GLenum type = luaL_checkinteger(L,8);
	const char *data = luaL_checkstring(L,9);
	glTexSubImage2D(target,level,x,y,w,h,fmt,type,data);
	CHECK_GL_ERROR(L)
	return 0;
}

static int
lPixelStorei(lua_State *L){
	GLenum name = luaL_checkinteger(L,1);
	GLint p = luaL_checkinteger(L,2);
	glPixelStorei(name,p);
	return 0;
}

static int
lEnable(lua_State *L){
	GLenum type = luaL_checkinteger(L,1);
	glEnable(type);
	return 0;
}

static int
lDisable(lua_State *L){
	GLenum type = luaL_checkinteger(L,1);
	glDisable(type);
	return 0;
}

static int
lBlendFunc(lua_State *L){
	GLenum sfac = luaL_checkinteger(L,1);
	GLenum dfac = luaL_checkinteger(L,2);
	glBlendFunc(sfac,dfac);
	return 0;
}

int
luaopen_gl(lua_State *L){
    luaL_Reg f[] = {
        {"init",linit},
        {"clear",lclear},
        {"vertexattr",lvertexattr},
        {"update_texture",lupdate_texture},

        {"SwapBuffer",lSwapBuffer},
        {"glViewport",lviewport},
        {"glCreateProgram",lglCreateProgram},
        {"glCreateShader",lglCreateShader},
        {"glShaderSource",lglShaderSource},
        {"glCompileShader",lglCompileShader},
        {"glAttachShader",lglAttachShader},
        {"glLinkProgram",lglLinkProgram},
        {"glUseProgram",lglUseProgram},
        {"glGenVertexArrays",lglGenVertexArrays},
        {"glBindVertexArray",lglBindVertexArray},
        {"glGenBuffers",lglGenBuffers},
        {"glBindBuffer",lglBindBuffer},
        {"glGenTextures",lglGenTextures},
        {"glBindTexture",lglBindTexture},
        {"glBufferData",lglBufferData},
        {"glBufferDatai2",lglBufferDatai2},
        {"glTexParameteri",lglTexParameteri},
        {"glDrawElements",lglDrawElements},
        {"glDrawArrays",lglDrawArrays},
        {"glUniformMatrix4fv",lglUniformMatrix4fv},
		{"glTexImage2D",lTexImage2D},
		{"glTexSubImage2D",lTexSubImage2D},
		{"glPixelStorei",lPixelStorei},
		{"glEnable",lEnable},
		{"glDisable",lDisable},
		{"glBlendFunc",lBlendFunc},
        {NULL,NULL}
    };
    luaL_newlib(L,f);
    _set_constant(L,"VERTEX_SHADER",GL_VERTEX_SHADER);
    _set_constant(L,"FRAGMENT_SHADER",GL_FRAGMENT_SHADER);
    _set_constant(L,"GL_FLOAT",GL_FLOAT);
    _set_constant(L,"GL_TRUE",GL_TRUE);
    _set_constant(L,"GL_FALSE",GL_FALSE);
    _set_constant(L,"TEX_RGBA8",TEX_RGBA8);
    _set_constant(L,"TEX_RGB",TEX_RGB);
    _set_constant(L,"TEX_RGBA4",TEX_RGBA4);
    _set_constant(L,"TEX_RGB565",TEX_RGB565);
    _set_constant(L,"TEX_A8",TEX_A8);
    _set_constant(L,"TEX_A4",TEX_A4);
    _set_constant(L,"GL_RGB",GL_RGB);
    _set_constant(L,"GL_RGBA",GL_RGBA);
    _set_constant(L,"GL_UNSIGNED_BYTE",GL_UNSIGNED_BYTE);
    _set_constant(L,"GL_UNSIGNED_SHORT",GL_UNSIGNED_SHORT);
    _set_constant(L,"GL_UNSIGNED_SHORT_565",GL_UNSIGNED_SHORT_5_6_5);
    _set_constant(L,"GL_UNSIGNED_SHORT_4444",GL_UNSIGNED_SHORT_4_4_4_4);
    _set_constant(L,"GL_ARRAY_BUFFER",GL_ARRAY_BUFFER);
    _set_constant(L,"GL_STREAM_DRAW",GL_STREAM_DRAW);
    _set_constant(L,"GL_ELEMENT_ARRAY_BUFFER",GL_ELEMENT_ARRAY_BUFFER);
    _set_constant(L,"GL_TRIANGLES",GL_TRIANGLES);
    _set_constant(L,"GL_POINT",GL_POINT);
    _set_constant(L,"GL_LINE",GL_LINE);
    _set_constant(L,"GL_TEXTURE_2D",GL_TEXTURE_2D);
    _set_constant(L,"GL_LINEAR",GL_LINEAR);
    _set_constant(L,"GL_TEXTURE_MIN_FILTER",GL_TEXTURE_MIN_FILTER);
    _set_constant(L,"GL_TEXTURE_MAG_FILTER",GL_TEXTURE_MAG_FILTER);
    _set_constant(L,"GL_TEXTURE_WRAP_S",GL_TEXTURE_WRAP_S);
    _set_constant(L,"GL_TEXTURE_WRAP_T",GL_TEXTURE_WRAP_T);
    _set_constant(L,"GL_CLAMP_TO_EDGE",GL_CLAMP_TO_EDGE);
    _set_constant(L,"GL_UNPACK_ALIGNMENT",GL_UNPACK_ALIGNMENT);
    _set_constant(L,"GL_ALPHA",GL_ALPHA);
    _set_constant(L,"GL_BLEND",GL_BLEND);
    _set_constant(L,"GL_SRC_ALPHA",GL_SRC_ALPHA);
    _set_constant(L,"GL_ONE_MINUS_SRC_ALPHA",GL_ONE_MINUS_SRC_ALPHA);
    return 1;
}

