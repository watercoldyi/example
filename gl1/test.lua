local window = require "window"
local glu = require "glu"
local ppm = require "glu.ppm"

local vs = [[
#version 300 es 
precision mediump float;
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
out vec2 fuv;
void main(void){
    gl_Position = vec4(pos.xy,1.0,1.0);
    fuv = uv;
}
]]

local fs = [[
#version 300 es 

precision mediump float;
in vec2 fuv;
out vec4 out_color;
uniform sampler2D texture0;
void main(void){
    out_color = texture2D(texture0,fuv);
}
]]

local program 
local dc
local vao,vbo,ebo 

local function create_shader(type,source)
    local id = glu.glCreateShader(type)
    glu.glShaderSource(id,source)
    glu.glCompileShader(id)
    glu.glAttachShader(id,program)
end

local function on_create()
    glu.init(dc)
    glu.glViewport(window.getsize())
    program = glu.glCreateProgram()
    create_shader(glu.VERTEX_SHADER,vs)
    create_shader(glu.FRAGMENT_SHADER,fs)
    glu.glLinkProgram(program)
    glu.glUseProgram(program)
    vao = glu.glGenVertexArrays()
    glu.glBindVertexArray(vao)
    vbo = glu.glGenBuffers()
    glu.glBindBuffer(glu.GL_ARRAY_BUFFER,vbo)
    ebo = glu.glGenBuffers()
    glu.glBindBuffer(glu.GL_ELEMENT_ARRAY_BUFFER,ebo)
    local tex = ppm.texture("sample")
    local t1 = glu.glGenTextures()
    glu.glBindTexture(t1)
    glu.update_texture(tex,t1)
    glu.glTexParameteri(glu.GL_TEXTURE_2D, glu.GL_TEXTURE_MIN_FILTER, glu.GL_LINEAR);
    glu.glTexParameteri(glu.GL_TEXTURE_2D, glu.GL_TEXTURE_MAG_FILTER, glu.GL_LINEAR);
    glu.glTexParameteri(glu.GL_TEXTURE_2D, glu.GL_TEXTURE_WRAP_S, glu.GL_CLAMP_TO_EDGE );
	glu.glTexParameteri(glu.GL_TEXTURE_2D, glu.GL_TEXTURE_WRAP_T, glu.GL_CLAMP_TO_EDGE );
end

local function mul(m1,m2)
    local m3 = {m1[1]*m2[1]+m1[2]*m2[3],m1[1]*m2[2]+m1[2]*m2[4]}
    return m3[1],m3[2]
end

local function rote(src,r)
    local k = math.pi / 180 * r
    local m = {
        math.cos(k),-math.sin(k),
        math.sin(k),math.cos(k)
    }
    src[1],src[2] = mul({src[1],src[2]},m)
    src[3],src[4] = mul({src[3],src[4]},m)
    src[5],src[6] = mul({src[5],src[6]},m)
    src[7],src[8] = mul({src[7],src[8]},m)
end

local function tex_draw(r)
    local pos = {
        -0.5,-0.5,
        -0.5,0.5,
        0.5,0.5,
        0.5,-0.5,
        -- uv
        0.34,0.25,
        0.34,0.0,
        0.57,0.0,
        0.57,0.25,
    }
    rote(pos,r)
    local index = {
        0,1,3,
        2,1,3
    }
    glu.glBufferData(glu.GL_ARRAY_BUFFER,pos,glu.GL_STREAM_DRAW)
    glu.glBufferDatai2(glu.GL_ELEMENT_ARRAY_BUFFER,index,glu.GL_STREAM_DRAW)
    glu.vertexattr(0,2,glu.GL_FLOAT,glu.GL_FALSE,8,0)
    glu.vertexattr(1,2,glu.GL_FLOAT,glu.GL_FALSE,8,32)
end


local b = 0
local function on_idle()
    glu.clear(0)
    tex_draw(b)
    glu.glDrawElements(glu.GL_TRIANGLES,6,glu.GL_UNSIGNED_SHORT,0)
    glu.SwapBuffer(dc)
    b = (b + 1) % 360
end

window.create("texture",1280,960)
window.on("create",on_create)
window.on("idle",on_idle)
window.show()
dc = window.getdc()
window.loop()