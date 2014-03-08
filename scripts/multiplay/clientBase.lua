--a client base object
local _ENV = mkmodule('hack.scripts.multiplay.clientBase')

local socket=require "plugins.luasocket"

local buffer=require('hack.scripts.multiplay.buffer').buffer
local messages=require('hack.scripts.multiplay.messages').MSG_TYPES
local PROTOCOL_VERSION=require('hack.scripts.multiplay.messages').PROTOCOL_VERSION

client=defclass(client)
client.ATTRS={
	initControls=DEFAULT_NIL,
	controls=DEFAULT_NIL,
	host="localhost",
	socket=1444
}
function client:chat(buf)
	print("chat:"..buf:extract('string'))
end
function client:info(buf)
	print("serverInfo:"..buf:extract('string'))
end
function client:version(buf)
	local pVer=buf:extract()
	if pVer~=PROTOCOL_VERSION then
		print("Fatal failure, protocol version mismatch")
		self:shutdown()
		return
	end
	print("Version received:"..pVer)
	--print("Supported modules:")
	self:sendVersion()
end
function client:received(id,size,data)
	local buf=self.buffer
	
	buf:receive(data,size)
	local msgType=buf:extract()
	print(id, "recieved", size,"bytes", "msg type=",msgType)
	if self.controls[msgType] then
		self.controls[msgType].fun(buf)
	end
end

function client:disconnected()
	print("Connection terminated")
end

function client:init(args)
	self.buffer=buffer{bufferSize=255}
	socket.onDataRecieved.multiplay=self:callback("received")
	socket.onDisconnect.multiplay=self:callback("disconnected")
	socket.lua_sock_connect(self.host,self.socket)
	self.controls=self.controls or {}
	self.controls[messages.CHAT]={fun=self:callback("chat")}
	
	self.controls[messages.INFO]={fun=self:callback("info")}
	self.controls[messages.ERROR]={fun=self:callback("info")}
	self.controls[messages.ERROR_FATAL]={fun=self:callback("info")}
	self.controls[messages.VERSION]={fun=self:callback("version")}
	for k,control in pairs(self.initControls or {}) do
		self:addControl(control)
	end
end
function client:addControl(control)
	for id,fun in pairs(control.supports()) do
		self.controls[id]={c=control,fun=dfhack.curry(fun,control)}
	end
end
function client:sendLogin(name,pass)
	local buf=self.buffer
	buf:append(messages.LOGIN)
	buf:append(name)
	buf:append(pass)
	buf:send(-1)
end
function client:sendVersion()
	local buf=self.buffer
	buf:append(messages.VERSION)
	buf:append(PROTOCOL_VERSION)
	buf:append(#self.controls)
	for k,v in ipairs(self.controls) do
		buf:append(v.ID)
	end
	buf:send(-1)
	if self.onDoneInit then
		self.onDoneInit()
	end
end
function client:shutdown()
	socket.lua_sock_disconnect(-1)
end
return _ENV