-- a free view observer
local _ENV = mkmodule('hack.scripts.multiplay.mod_control_view')
local gui=require("gui")
local offscreen= require 'plugins.offscreen'

local buffer=require('hack.scripts.multiplay.buffer').buffer
local messages=require('hack.scripts.multiplay.messages').MSG_TYPES

local mods=require 'hack.scripts.multiplay.modules'
--resend view on change of viewscreen
local RESEND_ON_CHANGE=false
server_saved_views={} --saved views by user name

control_view=mods.make_mod(control_view,'control_view',1)
local server=control_view.server
function server:init(args)
	
end
function server:player_join(player)
	server_saved_views[player.name]=server_saved_views[player.name] or {}
	player.view=server_saved_views[player.name]
end
function server:center_on(player,x,y,z)
	local view=player.view
	local w=view.width or 20
	local h=view.height or 20
	view.x1=x-w/2
	view.x2=view.x1+w-1
	view.y1=y-h/2
	view.y2=view.y1+h-1
	view.z=z
	if RESEND_ON_CHANGE then
		self:serve_view(player)
	end
end
function server:set_view(player,rect,z)
	local view=player.view
	--manual assign not to loose the ref
	view.x1=rect.x1
	view.x2=rect.x2
	view.y1=rect.y1
	view.y2=rect.y2
	view.z=rect.z or z
	view.width=rect.width
	view.height=rect.height
	--TODO clamp view
	if RESEND_ON_CHANGE then
		self:serve_view(player)
	end
end

function server:move_view(player,dx,dy,dz)
	local view=player.view
	dz=dz or 0
	view.x1=view.x1+dx
	view.x2=view.x2+dx
	
	view.y1=view.y1+dy
	view.y2=view.y2+dy
	
	view.z=view.z+dz
	
	if RESEND_ON_CHANGE then
		self:serve_view()
	end
end
local function appendScreen(screen,w,h,buf)
	for x=0,w-1 do
	for y=0,h-1 do
		local idx=(x)*h+(y)
		--local bufIdx=idx*7+offset
		if screen[idx] then
			buf:append(screen[idx].tile,32)
			buf:append(screen[idx].fg)
			buf:append(screen[idx].bg)
			if screen[idx].bold then --srsly if this needs to be done i dunno...
				buf:append(1)
			else
				buf:append(0)
			end
		end
	end
	end
end
function server:serve_view(player)
	local view=player.view
	local buf=self.buffer
	local w=view.width
	local h=view.height
	local buf = player.buffer
	buf:reset()
	buf:append(messages.CONTROL_VIEW_UPDATE)
	buf:append(w,16)
	buf:append(h,16)
	local offBuf=offscreen.draw(view,view.z)
	appendScreen(offBuf,w,h,buf)
	--print("sending screen:",w,h,w*h*7,#offBuf*7)
	buf:send(player.client)
end
--message parsing
function server:msg_set(buf,player)
	local x1,x2,y1,y2,z
	x1=buf:extract(nil,16)
	y1=buf:extract(nil,16)
	x2=buf:extract(nil,16)
	y2=buf:extract(nil,16)
	z=buf:extract(nil,16)
	local rect=gui.mkdims_xy(x1,y1,x2,y2)
	self:set_view(player,rect,z) 
	
	if RESEND_ON_CHANGE then
		self:serve_view(player)
	end
end
function server:msg_update(buf,player)
	self:serve_view(player)
end
function server:msg_move(buf,player)
	local dx,dy,dz
	dx=buf:extract(nil,16)
	dy=buf:extract(nil,16)
	dz=buf:extract(nil,16)
	self:move_view(player,dx,dy,dz)
end

-- A client control
control_view.client.ATTRS={
	readyFunction=DEFAULT_NIL, -- a callback when the new screen is ready
	dirty=false,	--the screen got updated
}
local client=control_view.client
function client:set_viewscreen(rect,z)
	local buf=self.buffer
	buf:reset()
	buf:append(messages.CONTROL_VIEW_SET)
	buf:append(rect.x1,16)
	buf:append(rect.y1,16)
	buf:append(rect.x2,16)
	buf:append(rect.y2,16)
	buf:append(z,16)
	buf:send()
end
function client:move_viewscreen(dx,dy,dz)
	local buf=self.buffer
	buf:reset()
	buf:append(messages.CONTROL_VIEW_MOVE)
	buf:append(dx,16)
	buf:append(dy,16)
	buf:append(dz,16)
	buf:send()
end
function client:render()
	local buf=self.buffer
	buf:reset()
	buf:append(messages.CONTROL_VIEW_UPDATE)
	self.dirty=false
	buf:send()
end
--message parsing
function client:msg_update(buf)
	self.dirty=true
	local screen=self.screen or {}
	local w=buf:extract(nil,16)-1
	local h=buf:extract(nil,16)-1
	screen.w=w
	screen.h=h
	for x=0,w-1 do
	for y=0,h-1 do
		local idx=(x)*h+(y)
		screen[idx]={tile=buf:extract(nil,32),fg=buf:extract(),bg=buf:extract(),bold=buf:extract()==1}
	end
	end
	self.screen=screen
	if self.readyFunction then
		self.readyFunction(screen)
	end
end

return _ENV