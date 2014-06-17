--server for df multiplayer
passwords=passwords or {} -- username/password hold, TODO improve

local socket=require "plugins.luasocket"
args={...}

local HOST="localhost"
local SOCKET=1444
-- max width and height
local W=255
local H=255
local TILESIZE=4+1+1+1

local buffer=require('hack.scripts.multiplay.buffer').buffer
local messages=require('hack.scripts.multiplay.messages').MSG_TYPES
local PROTOCOL_VERSION=require('hack.scripts.multiplay.messages').PROTOCOL_VERSION
local mods=require('hack.scripts.multiplay.modules')


server=defclass(server)
server.ATTRS={
	init_controls=DEFAULT_NIL,
	clients={}, --clients have empty table if not logged in, have a non-empty controls table if sent the version info
	alive=true
}
function server:init(args)
	self.buffer=buffer{bufferSize=W*H*TILESIZE}
	
	self.sock=socket.tcp:bind(HOST,SOCKET)
	print("Server started on:"..HOST..":"..SOCKET)
	self.controls_by_id={}
	self.msg_callbacks={}
	for i,ctrl in ipairs(self.init_controls or {}) do
		local ctrl_obj=ctrl{server=self,buffer=self.buffer}
		self.controls_by_id[ctrl.ID]=ctrl_obj
		for msg,fun in pairs(ctrl.supports()) do
			print("Supports:",msg,messages[msg])
			self.msg_callbacks[msg]={obj=ctrl_obj,fun=fun}
		end
	end
	
	self.players={}
end
function server:accept_clients()
	repeat
		local client=self.sock:accept()
		if client then
			self:newClient(client)
		end
	until client==nil
end
function server:controler_func(fname,...)
	for id,ctrl in ipairs(self.controls_by_id) do
		ctrl:invoke_after(fname,...)
	end
end
function server:tick()
	--accept all new clients
	self:accept_clients()
	--do module ticks
	self:controler_func('tick')
	--process received messages
	for client,_ in pairs(self.clients) do
		local ok=dfhack.safecall(self:callback("tryReceive",client))
		if not ok then
			self:disconnect(client)
		end
	end
end
function server:newClient(client)
	print("New client connected:",client.client_id)
	self.clients[client]={controls={}}
	self:controler_func('new_client',client)
end
function server:tryReceive(client)
	local buf=self.buffer
	buf.client=client
	local size=buf:receive()
	if not size then
		return
	end
	local msg=buf:extract()
	--print("data received, size="..size.." msg="..tostring(msg))
	if msg==messages.VERSION then
		self:initClient(client,buf)
	elseif msg==messages.LOGIN then
		self:login(client,buf)
	elseif msg==messages.CHAT then
		self:chat(client,buf)
	else
		local control=self.msg_callbacks[msg]
		if control~=nil and self.clients[client].controls[control.obj] then
			control.fun(control.obj,buf,self.players[client])
		else
			error("Unsupported message received:"..tostring(msg))
		end
	end
end
function server:disconnect(client) --how to?
	print("Client disconnected:",client.client_id)

	if self.clients[client] and self.players[client] then
		self:controler_func('player_left',self.players[client])
		self.players[client].client=nil
		self.players[client]=nil
		
	end

	self.clients[client]=nil
	client:close()

end
function server:initClient(client,buf)
	local pVer=buf:extract()
	if pVer~=PROTOCOL_VERSION then
		self:sendInfo(client,"Protocol version mismatch: expecting:"..PROTOCOL_VESION.." got:"..pVer)
		return
	end
	local numControls=buf:extract()
	print("Client:"..client.client_id.." will be handling:"..numControls.." controls")
	for i=1,numControls do
		local ctrlId=buf:extract()
		print("Loading control id=",ctrlId)
		local control=self.controls_by_id[ctrlId]
		if control==nil then
			qerror("Unsupported control id:",tostring(ctrlId))
		end
		self.clients[client].controls[control]=true
	end
	
end
function server:login(client,buf)
	local name=buf:extract('string')
	local pass=buf:extract('string')
	local obj_hold={}
	print("Player logging in:"..name)
	if passwords[name]==nil or passwords[name]==pass then
		local player={name=name,id=client.client_id,buffer=self.buffer,client=client}
		self.clients[client].player=player
		self.players[client]=player
		passwords[name]=pass
		self:controler_func('player_join',player)

	else
		self:sendInfo(client,"Invalid password",true)
	end
end
function server:chat(client,buf)
	local msg=buf:extract('string')
	if self.players[client] then
		local name=self.players[client].name
		local buf=self.buffer
		msg=string.format("[%s] %s",name,msg)
		print(msg)
		buf:reset()
		for k,v in pairs(self.players) do
			buf:append(messages.CHAT)
			buf:append(msg)
			buf:send(k)
		end
	end
end
function server:sendInfo(client,str,isFatal)
	local buf=self.buffer
	buf:reset() --needed if in a middle of forming a response or sth...
	buf.client=client
	if isFatal==nil then
		buf:append(messages.INFO)
		buf:append(str)
		buf:send()
	elseif isFatal==false then
		buf:append(messages.ERROR)
		buf:append(str)
		buf:send()
	elseif isFatal==true then
		buf:append(messages.ERROR_FATAL)
		buf:append(str)
		buf:send()
		self:disconnect(client)
	end
end
function server:sendVersion(client)
	local buf=self.buffer
	buf:reset() --optional
	buf.client=client
	buf:append(messages.VERSION)
	buf:append(PROTOCOL_VERSION)
	buf:append(#self.controls)
	for k,v in ipairs(self.controls) do
		buf:append(v.ID)
	end
	buf:send()
end
function server:shutdown()
	print("Server shuting down")
	for k,v in pairs(self.clients) do
		k:close()
	end
	self.buffer=nil
	self.sock:close()
	self.alive=false
end


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
	serverInstance = server{
	init_controls={
		mods.require_server('control_view'),
		mods.require_server('dwarf_watch'),
		},
	}
	local function loopy()
		serverInstance:tick()
		if serverInstance.alive then
			dfhack.timeout(5,"frames",loopy)
		else
			print("server died")
		end
	end
	loopy()
end
