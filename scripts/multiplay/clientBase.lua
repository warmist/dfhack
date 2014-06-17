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
	socket=1444,
	alive=true
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
function client:received()
	local buf=self.buffer
	
	local msgType=buf:extract()
	--print("Recieved", buf:size(),"bytes", "msg type=",msgType)
	if self.controls[msgType] then
		self.controls[msgType].fun(buf)
	else
		print("unsuported msg:",msgType)
	end
end

function client:disconnected()
	print("Connection terminated")
end

function client:init(args)
	self.buffer=buffer{bufferSize=255}

	--socket.onDataRecieved.multiplay=self:callback("received")
	--socket.onDisconnect.multiplay=self:callback("disconnected")
	self.sock=socket.tcp:connect(self.host,self.socket)
	print("Socket:",self.sock)
	self.buffer.client=self.sock
	self.controls_by_id={}
	self.controls=self.controls or {}
	self.controls[messages.CHAT]={fun=self:callback("chat")}
	
	self.controls[messages.INFO]={fun=self:callback("info")}
	self.controls[messages.ERROR]={fun=self:callback("info")}
	self.controls[messages.ERROR_FATAL]={fun=self:callback("info")}
	self.controls[messages.VERSION]={fun=self:callback("version")}
	for k,control in pairs(self.initControls or {}) do
		print("Adding control:",k)
		self:addControl(control)
	end
end
function client:addControl(control)
	table.insert(self.controls_by_id,control.ID)
	control.buffer=control.buffer or self.buffer
	for id,fun in pairs(control.supports()) do
		self.controls[id]={c=control,fun=dfhack.curry(fun,control)}
	end
end
function client:sendLogin(name,pass)
	local buf=self.buffer
	buf:reset()
	buf:append(messages.LOGIN)
	buf:append(name)
	buf:append(pass)
	buf:send()
end
function client:sendVersion()
	local buf=self.buffer
	buf:reset()
	buf:append(messages.VERSION)
	buf:append(PROTOCOL_VERSION)
	buf:append(#self.controls_by_id)
	print("initing control count:",#self.controls_by_id)
	for _,id in ipairs(self.controls_by_id) do
		buf:append(id)
	end
	buf:send()
	if self.onDoneInit then
		self.onDoneInit()
	end
end
function client:sendChat( text )
	local buf=self.buffer
	buf:reset()
	buf:append(messages.CHAT)
	buf:append(text)
	buf:send()
end
function client:shutdown()
	self.sock:close()
	self.alive=false
end
function client:tick( )
	local ok=safecall(function()
	local size=self.buffer:receive()
	if size then
		self:received()
	end
	end)
	if not ok then
		self:shutdown()
	end
end
return _ENV