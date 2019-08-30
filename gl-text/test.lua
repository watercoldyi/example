local gl = require "gl"
local window = require "window"
local font = require "font"

local WINDOW_W = 1280
local WINDOW_H = 720 
local FONT_SIZE = 32 
local SCREEN_XSCALE = 2.0 / WINDOW_W 
local SCREEN_YSCALE = -2.0 / WINDOW_H 
local TEXT_TEX_W = 1024
local TEXT_TEX_H = 1024
local TEX_XSCALE = 1 / TEXT_TEX_W
local TEX_YSCALE = 1 / TEXT_TEX_H

local _font = font.font_create(FONT_SIZE)
local _dfont = font.dfont_create(1024,1024)	

local _vs = [[
#version 300 es
precision mediump float;
layout (location = 0) in vec2 v;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec4 color;

out vec2 vtexcoord;
out vec4 vcolor;
void main(){
	gl_Position = vec4(v.xy,0.0,1.0) + vec4(-1.0,1.0,0.0,0.0);
	vcolor = color;
	vtexcoord = texcoord;
}

]]

local _fs = [[
#version 300 es
precision mediump float;
in vec2 vtexcoord;
in vec4 vcolor;
out vec4 color;
uniform sampler2D texture0;

void main(){
	float c = texture2D(texture0,vtexcoord).w;
	float alpha = clamp(c,0.0,0.5) * 2.0;
	color = vec4(vcolor.r * alpha,vcolor.g * alpha,vcolor.b * alpha,vcolor.a);
}
]]

local _program,_dc
local _vbuffer = {}

local function _create_shader(t,source)
	local id = gl.glCreateShader(t)
	gl.glShaderSource(id,source)
	gl.glCompileShader(id)
	gl.glAttachShader(id,_program)
end

local function _init_ebuffer()
	local ebuf = {}
	local ebo = gl.glGenBuffers()
	gl.glBindBuffer(gl.GL_ELEMENT_ARRAY_BUFFER,ebo)
	for i = 0,128 do
		table.insert(ebuf,i * 4)	
		table.insert(ebuf,i * 4 + 1)	
		table.insert(ebuf,i * 4 + 2)	
		table.insert(ebuf,i * 4)	
		table.insert(ebuf,i * 4 + 3)	
		table.insert(ebuf,i * 4 + 2)	
	end
	gl.glBufferDatai2(gl.GL_ELEMENT_ARRAY_BUFFER,ebuf,gl.GL_STREAM_DRAW)
end

local function on_create()
	gl.init(_dc)
	gl.glViewport(window.getsize())
	_program = gl.glCreateProgram()
	_create_shader(gl.VERTEX_SHADER,_vs)
	_create_shader(gl.FRAGMENT_SHADER,_fs)
	gl.glLinkProgram(_program)
	gl.glUseProgram(_program)
	local vao = gl.glGenVertexArrays()
	gl.glBindVertexArray(vao)
	local vbo = gl.glGenBuffers()
	gl.glBindBuffer(gl.GL_ARRAY_BUFFER,vbo)
	_init_ebuffer()
	local tex = gl.glGenTextures()
	gl.glBindTexture(tex)
	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_LINEAR)
    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_LINEAR)
    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE )
	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE )
	gl.glPixelStorei(gl.GL_UNPACK_ALIGNMENT,1)
	gl.glTexImage2D(gl.GL_TEXTURE_2D,0,gl.GL_ALPHA,TEXT_TEX_W,TEXT_TEX_H,0,gl.GL_ALPHA,gl.GL_UNSIGNED_BYTE,nil)
	gl.vertexattr(0,2,gl.GL_FLOAT,gl.GL_FALSE,32,0)
	gl.vertexattr(1,2,gl.GL_FLOAT,gl.GL_FALSE,32,8)
	gl.vertexattr(2,4,gl.GL_FLOAT,gl.GL_FALSE,32,16)
end

local function _pack_vertex(vx,vy,tx,ty,color)
	return {
		vx * SCREEN_XSCALE, vy * SCREEN_YSCALE,
		tx * TEX_XSCALE, ty * TEX_YSCALE,
		(color >> 24) / 255, --r
		((color & 0x00ff0000) >> 16) / 255, --g
		((color & 0x0000ff00) >> 8) / 255,--b,
		color & 0x000000ff / 255, -- a 
	}
end


local function _pack(x,y,size,rect,color)
	local w = rect.w * size / FONT_SIZE 
	local h = rect.h * size / FONT_SIZE 
	local obj = {}
	obj[1] = _pack_vertex(x,y,rect.x,rect.y,color)
	obj[2] = _pack_vertex(x+w,y,rect.x + rect.w,rect.y,color)
	obj[3] = _pack_vertex(x+w,y + h,rect.x + rect.w,rect.y + rect.h,color)
	obj[4] = _pack_vertex(x,y + h,rect.x,rect.y + rect.h,color)
	table.move(obj[1],1,8,#_vbuffer + 1,_vbuffer)
	table.move(obj[2],1,8,#_vbuffer + 1,_vbuffer)
	table.move(obj[3],1,8,#_vbuffer + 1,_vbuffer)
	table.move(obj[4],1,8,#_vbuffer + 1,_vbuffer)
end

local function _draw_char(sx,sy,unicode,size,color)
	local x,y,w,h = _dfont:lookup(unicode,size,0)
	if not x then
		w,h = _font:size(unicode)
		local data = _font:glyph(unicode)
		x,y,w,h = _dfont:insert(unicode,size,w,h,0)
		gl.glTexSubImage2D(gl.GL_TEXTURE_2D,0,x,y,w,h,gl.GL_ALPHA,gl.GL_UNSIGNED_BYTE,data)
	end
	local rect = {x = x,y = y,w = w,h = h}
	_pack(sx,sy,size,rect,color)
end

local function _commit()
	local n = #_vbuffer / 32 
	gl.glBufferData(gl.GL_ARRAY_BUFFER,_vbuffer,gl.GL_STREAM_DRAW)
	gl.glDrawElements(gl.GL_TRIANGLES,n*6,gl.GL_UNSIGNED_SHORT,0)
	_vbuffer = {}
end


local function _label(x,y,str,size,color)
	local cx,cy = x,y	
	local maxh = 0
	for _,code in utf8.codes(str) do
		local w,h = _font:size(code)
		if maxh < h then maxh = h end
		_draw_char(cx,cy,code,size,color)
		cx = cx + w *size / FONT_SIZE  
	end
	return y+maxh 
end	

local function on_idle()
	gl.clear(0)
	local x,y = 100,100
	y = _label(x,y,"床前明月光",30,0x1364cb00) + 15 
	y = _label(x,y,"疑是地上霜",40,0xff7f7f00) + 15 
	y = _label(x,y,"举头望明月",50,0xff7f0000) + 15 
	y = _label(x,y,"低头思故乡",60,0xff00ff00) + 15 
	_commit()
	gl.SwapBuffer(_dc)
end

window.create("font",WINDOW_W,WINDOW_H)
window.on("create",on_create)
window.on("idle",on_idle)
_dc = window.getdc()
window.show()
window.loop()


