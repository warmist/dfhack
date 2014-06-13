-- a helper to constructing multiplayer modules
--[[
    module may implement messages that it can accept in form of

    function sth.client:msg_<message_suffix>()
    end

    where message_suffix is from messages module and e.g. in 'VIEWSCREEN_UPDATE' it's 'update'
]]
local _ENV = mkmodule('hack.scripts.multiplay.modules')

local messages=require('hack.scripts.multiplay.messages').MSG_TYPES
ID_MAP=ID_MAP or {}

mod_server=defclass(mod_server)
mod_server.ATTRS={
    player=DEFAULT_NIL,
    server=DEFAULT_NIL
}
function mod_server:init(args)
    if self.player==nil then
        error("Invalid player")
    end
    if self.server==nil then
        error("Invalid server")
    end
end

mod_client=defclass(mod_client)
mod_client.ATTRS={
    buffer=DEFAULT_NIL,
}
--[[
can't check due to cyclic dependency
function mod_client:init( args )
    if self.buffer==nil then
        error("Invalid buffer")
    end
end]]

local function get_supports(class,name)
    local ret={}
    for k,v in pairs(class) do
        if type(k)=="string" then
            local fname=k:match("msg_(%w+)")
            if fname then
                ret[messages[string.upper(name.."_"..fname)]]=v
            end
        end
    end
    return ret
end
function make_mod(oldclass,name,ID)
    local ret=oldclass or {}
    ret.server=defclass(ret.server,mod_server)
    ret.server.ID=ID
    ret.server.supports=function()
        return get_supports(ret.server,name)
    end
    ret.client=defclass(ret.client,mod_client)
    ret.client.ID=ID
    ret.client.supports=function()
        return get_supports(ret.client,name)
    end
    ret.name=name
    if ID_MAP[ID] then
        if ID_MAP[ID].name and ID_MAP[ID].name~=name then
            error("Overwriting different module is not allowed")
        end
    end
    ID_MAP[ID]=ret
    ID_MAP[name]=ret
    return ret
end
function get_client_class( ID )
    return ID_MAP[ID].client
end
function get_server_class( ID )
   return ID_MAP[ID].server
end
function require_server( name )
    return require('hack.scripts.multiplay.mod_'..name)[name].server
end
function require_client( name )
    return require('hack.scripts.multiplay.mod_'..name)[name].client
end
return _ENV