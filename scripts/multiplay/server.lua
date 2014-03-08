--server for df multiplayer
players=players or {} -- username/password hold, TODO improve

local socket=require "plugins.luasocket"
args={...}

local offscreen= require 'plugins.offscreen'

local HOST="localhost"
local SOCKET=1444
-- max width and height
local W=255
local H=255
local TILESIZE=4+1+1+1

local buffer=require('hack.scripts.multiplay.buffer').buffer
local messages=require('hack.scripts.multiplay.messages').MSG_TYPES
local PROTOCOL_VERSION=require('hack.scripts.multiplay.messages').PROTOCOL_VERSION
server=defclass(server)
server.ATTRS={
	controls=DEFAULT_NIL,
	clients={} --clients have empty table if not logged in, have a non-empty controls table if sent the version info
}
function server:init(args)
	self.buffer=buffer{bufferSize=W*H*TILESIZE}
	socket.onNewClient.multiplay=self:callback("newClient")
	socket.onDataRecieved.multiplay=self:callback("dataReceived")
	socket.onDisconnect.multiplay=self:callback("disconnect")
	socket.lua_sock_listen(HOST,SOCKET)
	print("Server started on:"..HOST..":"..SOCKET)
end

function server:newClient(id)
	print("New client connected:",id)
	self.clients[id]={controls={}}
	self:sendVersion(id)
end
function server:dataReceived(id,size,data)
	local buf=self.buffer
	buf:receive(data,size)
	local msg=buf:extract()
	print("data received, size="..size.." msg="..tostring(msg))
	if msg==messages.VERSION then
		self:initClient(id,buf)
	elseif msg==messages.LOGIN then
		self:login(id,buf)
	elseif msg==messages.CHAT then
		self:chat(id,buf)
	else
		local control=self.clients[id].controls[msg]
		if control~=nil then
			control.fun(control.obj,buf)
		else
			error("Unsupported message received:"..tostring(msg))
		end
	end
end
function server:disconnect(id)
	print("Client disconnected:",id)
	--TODO maybe controls want to know that?
	self.clients[id]=nil
end
function server:initClient(id,buf)
	local pVer=buf:extract()
	if pVer~=PROTOCOL_VERSION then
		self:sendInfo(id,"Protocol version mismatch: expecting:"..PROTOCOL_VESION.." got:"..pVer)
		return
	end
	local numControls=buf:extract()
	print("Client:"..id.." will be handling:"..numControls.." controls")
	for i=1,numControls do
		local ctrlId=buf:extract()
		local control=self.controls[ctrlId]
		for k,v in pairs(control.supports()) do
			self.clients[id].controls[k]={fun=v,class=control}
		end
	end
	
end
function server:login(id,buf)
	local name=buf:extract('string')
	local pass=buf:extract('string')
	print("Player logging in:"..name)
	if players[name]==nil or players[name]==pass then
		self.clients[id].player={name=name,id=id,buffer=self.buf}
		for k,v in pairs(self.clients[id].controls) do
			v.obj=v.class{player=self.clients[id].player}
		end
		players[name]=pass
	else
		self:sendInfo(id,"Invalid password",true)
	end
end
function server:sendInfo(id,str,isFatal)
	local buf=self.buffer
	buf:reset() --needed if in a middle of forming a response or sth...
	if isFatal==nil then
		buf:append(messages.INFO)
		buf:append(str)
		buf:send(id)
	elseif isFatal==false then
		buf:append(messages.ERROR)
		buf:append(str)
		buf:send(id)
	elseif isFatal==true then
		buf:append(messages.ERROR_FATAL)
		buf:append(str)
		buf:send(id)
		socket:lua_sock_disconnect(id)
	end
end
function server:sendVersion(id)
	local buf=self.buffer
	buf:reset() --optional
	buf:append(messages.VERSION)
	buf:append(PROTOCOL_VERSION)
	buf:append(#self.controls)
	for k,v in ipairs(self.controls) do
		buf:append(v.ID)
	end
	buf:send(id)
end
function server:shutdown()
	print("Server shuting down")
	for k,v in pairs(self.clients) do
		socket.lua_sock_disconnect(k)
	end
	self.buffer=nil
	socket.lua_sock_disconnect(-2)
end
local ctrl_view_free=require('hack.scripts.multiplay.control_view_free').control_view.server
if args[1] and args[1]=="shutdown" then
	if serverInstance then
		serverInstance:shutdown()
	end
	serverInstance=nil
else
	if serverInstance then
		serverInstance:shutdown()
		serverInstance=nil
	end
	serverInstance = server{controls={ctrl_view_free}}
end
