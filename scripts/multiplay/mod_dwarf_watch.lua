local _ENV = mkmodule('hack.scripts.multiplay.mod_dwarf_watch')

local buffer=require('hack.scripts.multiplay.buffer').buffer
local messages=require('hack.scripts.multiplay.messages').MSG_TYPES
local mods=require 'hack.scripts.multiplay.modules'

dwarf_watch=mods.make_mod(dwarf_watch,'dwarf_watch',2)
--SERVER
dwarf_watch.server.ATTRS={
    view_module=DEFAULT_NIL
}

function dwarf_watch.server:init(args)
    if self.view_module==nil then error("View module not set") end
    --check if dwarf is assigned, if not assign one

    --send new dwarf and maybe dwarf_info messages
end

function dwarf_watch.server:tick(args)
    --if has enabled follow AND has valid unit assigned, center view on unit
end
function dwarf_watch.server:send_info(info_string)
    
end
--message parsing
function dwarf_watch.server:msg_new_dwarf()
    --assign new dwarf and send it's id+fullname to client
end
function dwarf_watch.server:msg_follow()
   --get 'enable' flag and set it 
end
--[=[ this could be some selective way of asking for some dwarf info (e.g. view his stats etc...)
function dwarf_watch.server:msg_dwarf_info()
   
end
]=]

--CLIENT

dwarf_watch.client.ATTRS={
    onNewDwarf=DEFAULT_NIL, -- a callback when the new dwarf is assigned
    onDwarfInfo=DEFAULT_NIL, -- a callback when the new dwarf info is received
}
function dwarf_watch.client:msg_new_dwarf()

end

function dwarf_watch.client:msg_dwarf_info()
   
end
return _ENV