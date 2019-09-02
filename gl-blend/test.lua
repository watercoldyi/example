local gl = require "gl"
local window = require "window"
local stbi = require "stbi"

local WINDOW_W = 640 
local WINDOW_H = 480 
local SCREEN_XSCALE = 2 / WINDOW_W
local SCREEN_YSCALE = -2 / WINDOW_H

local _program,_dc
local _vbuf = {}

local _vs = [[
#version 300 es
precision mediump float;
layout (location = 0) in vec4 v;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec3 mask;
layout (location = 3) in float alpha;
out vec2 v_texcoord;
out vec3 v_mask;
out float v_alpha;
void main(){
	gl_Position = v + vec4(-1,1,0,0);
	v_texcoord = texcoord;
	v_mask = mask;
	v_alpha = alpha;
}

]]

local _fs = [[
#version 300 es
precision mediump float;
in vec2 v_texcoord;
in vec3 v_mask;
in float v_alpha;
uniform sampler2D texture0;
void main(){
	vec4 pixel = texture2D(texture0,v_texcoord);
	float alpha = pixel.rgb == v_mask ? 0.0 : v_alpha; 
	gl_FragColor = vec4(pixel.rgb,alpha);
}

]]

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
	for i = 0,1024 do
		table.insert(ebuf,i * 4)	
		table.insert(ebuf,i * 4 + 1)	
		table.insert(ebuf,i * 4 + 2)	
		table.insert(ebuf,i * 4)	
		table.insert(ebuf,i * 4 + 3)	
		table.insert(ebuf,i * 4 + 2)	
	end
	gl.glBufferDatai2(gl.GL_ELEMENT_ARRAY_BUFFER,ebuf,gl.GL_STREAM_DRAW)
end

local function _pack_vertex(vx,vy,tx,ty,tscalex,tscaley,mask,alpha)
	return {
		vx * SCREEN_XSCALE, vy * SCREEN_YSCALE,0,1,
		tx * tscalex, ty * tscaley,
		((mask & 0xff0000) >> 16) / 255,
		((mask & 0x00ff00) >> 8) / 255,
		(mask & 0xff) / 255,
		alpha
	}
end

local function _pack_rect(vx,vy,vw,vh,tx,ty,tw,th,tscalex,tscaley,mask,alpha,buf)
	table.move(_pack_vertex(vx,vy,tx,ty,tscalex,tscaley,mask,alpha),1,10,#buf + 1,buf)
	table.move(_pack_vertex(vx + vw,vy,tx + tw,ty,tscalex,tscaley,mask,alpha),1,10,#buf + 1,buf)
	table.move(_pack_vertex(vx + vw,vy + vh,tx + tw,ty + th,tscalex,tscaley,mask,alpha),1,10,#buf + 1,buf)
	table.move(_pack_vertex(vx,vy + vh,tx,ty+th,tscalex,tscaley,mask,alpha),1,10,#buf + 1,buf)
end

local function _commit()
	local n = #_vbuf / 40 
	gl.glBufferData(gl.GL_ARRAY_BUFFER,_vbuf,gl.GL_STREAM_DRAW)
	gl.glDrawElements(gl.GL_TRIANGLES,n*6,gl.GL_UNSIGNED_SHORT,0)
	_vbuf = {}
end

local function _draw1()
	gl.glBindTexture(1)
	local mask = 0xff00ff
	local alpha = 1.0
	for y = 0,30 do
		for x = 0,20 do
			local x,y = x * 64,y * 32
			_pack_rect(x-32,y-16,64,32,0,0,64,32,1/64,1/32,mask,alpha,_vbuf)
		end
	end
	_commit()
end

local function _draw2()
	gl.glBindTexture(2)
	local mask = 0xff00ff
	local alpha = 1.0
	local vb = {}
	for y = 0,30 do
		for x = 0,25 do
			local x,y = x * 64,y * 32
			_pack_rect(x,y,64,32,0,0,64,32,1/64,1/32,mask,alpha,_vbuf)
		end
	end
	_commit()
end

local function _draw_map()
	_draw1()
	_draw2()
end

local function _draw_man()
	gl.glBindTexture(3)
	local mask = 0xff00ff
	local alpha = 1.0
	local w,h,x,y = 50,73,200,150
	_pack_rect(x,y,w,h,0,0,w,h,1/50,1/73,mask,alpha,_vbuf)
	mask = 0
	_pack_rect(x+100,y,w,h,0,0,w,h,1/50,1/73,mask,alpha,_vbuf)
	_commit()
end

local function _draw_mz()
	gl.glBindTexture(4)
	local mask = 0 
	local alpha = 1.0
	local w,h,x,y = 50,80,200,250
	_pack_rect(x,y,w,h,0,0,w,h,1/w,1/h,mask,alpha,_vbuf)
	_pack_rect(x+100,y,w,h,0,0,w,h,1/w,1/h,mask,0.5,_vbuf)
	_commit()
end

local function _load_texture(file)
	local tex = gl.glGenTextures()
	gl.glBindTexture(tex)
	gl.glPixelStorei(gl.GL_UNPACK_ALIGNMENT,1);
	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_LINEAR)
    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_LINEAR)
    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE )
	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE )
	local data,w,h,n = stbi.load(file,0)
	gl.glTexImage2D(gl.GL_TEXTURE_2D,0,gl.GL_RGB,w,h,0,gl.GL_RGB,gl.GL_UNSIGNED_BYTE,data)
	stbi.free(data)
end

local function on_idle()
	gl.clear(0xabecbbff)
	_draw_map()
	_draw_man()
	_draw_mz()
	gl.SwapBuffer(_dc)
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
	gl.vertexattr(0,4,gl.GL_FLOAT,gl.GL_FALSE,40,0)
	gl.vertexattr(1,2,gl.GL_FLOAT,gl.GL_FALSE,40,16)
	gl.vertexattr(2,3,gl.GL_FLOAT,gl.GL_FALSE,40,24)
	gl.vertexattr(3,1,gl.GL_FLOAT,gl.GL_FALSE,40,36)
	_load_texture("tile1.bmp")
	_load_texture("tile2.bmp")
	_load_texture("man.bmp")
	_load_texture("mz.bmp")
	gl.glEnable(gl.GL_BLEND);
	gl.glBlendFunc(gl.GL_SRC_ALPHA,gl.GL_ONE_MINUS_SRC_ALPHA);
end


window.create("blend",WINDOW_W,WINDOW_H)
window.on("idle",on_idle)
window.on("create",on_create)
window.show()
_dc = window.getdc()
window.loop()
