local _ENV = mkmodule('plugins.luasocket')
local _funcs={}
for k,v in pairs(_ENV) do
    if type(v)=="function" then
        _funcs[k]=v
        _ENV[k]=nil
    end
end
local socket=defclass(socket)
socket.ATTRS={
    server_id=-1,
    client_id=-1,
}
function socket:close(  )
    if self.client_id==-1 then
        _funcs.lua_server_close(self.server_id)
    else
        _funcs.lua_client_close(self.server_id,self.client_id)
    end
end
function socket:setTimeout( sec,msec )
    msec=msec or 0
    _funcs.lua_socket_set_timeout(self.server_id,self.client_id,sec,msec)
end

local client=defclass(client,socket)
function client:init(args )
    local m=getmetatable(self)
    m.__gc=function(obj)
        print("collecting",self.server_id,self.client_id)
        obj:close()
    end
    setmetatable(self,m)
end
function client:receive( pattern )
    local pattern=pattern or "*l"
    local bytes=-1
    if type(pattern)== "number" then
        bytes=pattern
    end
    local ret=_funcs.lua_client_receive(self.server_id,self.client_id,bytes,pattern,false)
    if ret=="" then
        return
    else
        return ret
    end
end
function client:send( data ,size)
    if type(data)=="string" then
        _funcs.lua_client_send(self.server_id,self.client_id,data,false)
    else
        _funcs.lua_client_send_raw(self.server_id,self.client_id,data,size,false)
    end
end


local server=defclass(server,socket)
function server:accept()
    local id=_funcs.lua_server_accept(self.server_id,false)
    if id~=0 then
        return client{server_id=self.server_id,client_id=id}
    else
        return 
    end
end

tcp={}
function tcp:bind( address,port )
    local id=_funcs.lua_socket_bind(address,port)
    return server{server_id=id}
end
function tcp:connect( address,port )
    local id=_funcs.lua_socket_connect(address,port)
    return client{client_id=id}
end
--TODO garbage collect stuff
return _ENV