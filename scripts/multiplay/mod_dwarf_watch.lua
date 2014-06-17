local _ENV = mkmodule('hack.scripts.multiplay.mod_dwarf_watch')

local buffer=require('hack.scripts.multiplay.buffer').buffer
local messages=require('hack.scripts.multiplay.messages').MSG_TYPES
local mods=require 'hack.scripts.multiplay.modules'

dwarf_watch=mods.make_mod(dwarf_watch,'dwarf_watch',2)
--SERVER
dwarf_watch.server.ATTRS={
    view_module=DEFAULT_NIL
}
local server=dwarf_watch.server

function server:get_unit(player)
    if player.dwarf_watch.unit_id then
        local unit=df.unit.find(player.dwarf_watch.unit_id)
        if not unit then
            player.dwarf_watch.unit_id=nil
            return
        else
            return unit
        end
    end
end
local function valid_unit( u )
    return true --not picky... yet!
end
function server:assign_unit(player)    
    local TRIES=3 -- randomly try 3 units
    local units=df.global.world.units.active
    if #units==0 then --no units exist
        return
    end
    --try random first
    for i=1,TRIES do
        local choice=math.random(0,#units-1)
        local unit=units[choice]
        if valid_unit(unit) then
            player.dwarf_watch.unit_id=unit.id
            return unit
        end
    end
    --failed, try linear scan
    for _,unit in ipairs(units) do
        if valid_unit(unit) then
            player.dwarf_watch.unit_id=unit.id
            return unit
        end
    end
    --failed... all units might be dead or sth
end
function server:player_join(player)
    player.dwarf_watch=player.dwarf_watch or {}
    player.dwarf_watch.enabled=false

    local unit=self:get_unit(player)
    if not unit then
        unit=self:assign_unit(player)
        if not unit then
            print("Warning: assign failed")
        end
    end
    if unit then
        self:send_new_dwarf(player,unit)
        player.dwarf_watch.enabled=true
    end
    --maybe send dwarf_info messages
end
function server:init(args)
    local view_srv=mods.get_server_class('control_view').ID
    self.view_module=self.server.controls_by_id[view_srv]
    if self.view_module==nil then error("View module not set") end
end

function server:tick()
    --if has enabled follow AND has valid unit assigned, center view on unit
    for player,_ in pairs(self.players) do
        if player.dwarf_watch.enabled then
            local unit=self:get_unit(player)
            if unit then
                self.view_module:center_on(player,unit.pos.x,unit.pos.y,unit.pos.z)
            else
                player.dwarf_watch.enabled=false
                print("Warning: unit ref lost!")
            end
        end
    end
end

function server:send_info(info_string)
    --TODO
end
function server:send_new_dwarf(player,unit)
    unit=unit or self:get_unit()
    if not unit then
        error("Invalid unit!")
    end
    local buf=player.buffer
    buf:reset()
    buf:append(messages.DWARF_WATCH_NEW_DWARF)
    buf:append(unit.id,32)
    buf:append(dfhack.TranslateName(unit.name))
    buf:send(player.client)
end
--message parsing
function server:msg_new_dwarf(buf,player) --maybe allow id later? like i want THIS dwarf
    --assign new dwarf and send it's id+fullname to client
    local unit=self:assign_unit(player)
    if not unit then
        print("Warning: assign failed")
    else
        self:send_new_dwarf(player,unit)
    end
end
function server:msg_follow(buf,player)
   --get 'enable' flag and set it 
    local enable=buf:extract()
    if enable==0 then
        player.dwarf_watch.enabled=false
    else
        player.dwarf_watch.enabled=true
    end
end
--[=[ this could be some selective way of asking for some dwarf info (e.g. view his stats etc...)
function server:msg_dwarf_info()
   
end
]=]

--CLIENT

dwarf_watch.client.ATTRS={
    onNewDwarf=DEFAULT_NIL, -- a callback when the new dwarf is assigned
    onDwarfInfo=DEFAULT_NIL, -- a callback when the new dwarf info is received
}
function dwarf_watch.client:enable_follow(flag)
    local buf=self.buffer
    buf:reset()
    buf:append(messages.DWARF_WATCH_FOLLOW)
    if flag then
        buf:append(1)
    else
        buf:append(0)
    end
    buf:send()
end
function dwarf_watch.client:request_new_dwarf()
    local buf=self.buffer
    buf:reset()
    buf:append(messages.DWARF_WATCH_NEW_DWARF)
    buf:send()
end
--message parsing
function dwarf_watch.client:msg_new_dwarf()
    if onNewDwarf then
        local buf=self.buffer
        local id=buf:extract(nil,32)
        local name=buf:extract('string')
        onNewDwarf(id,name)
    end
end

function dwarf_watch.client:msg_dwarf_info()
    if onDwarfInfo then
        local buf=self.buffer
        local txt=buf:extract('string')
        onDwarfInfo(txt)
    end
end
return _ENV

-- body