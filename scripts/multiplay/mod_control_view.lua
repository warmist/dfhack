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

function control_view.server:init(args)
	server_saved_views[args.player.name]=server_saved_views[args.player.name] or {}
	self.view=server_saved_views[args.player.name]
	self.buffer=args.player.buffer or buffer{bufferSize=255*255*7,client=args.player.client}
end

function control_view.server:set_view(rect,z)
	--manual assign not to loose the ref
	self.view.x1=rect.x1
	self.view.x2=rect.x2
	self.view.y1=rect.y1
	self.view.y2=rect.y2
	self.view.z=rect.z or z
	self.view.width=rect.width
	self.view.height=rect.height
	--TODO clamp view
	if RESEND_ON_CHANGE then
		self:serve_view()
	end
end

function control_view.server:move_view(dx,dy,dz)
	dz=dz or 0
	self.view.x1=self.view.x1+dx
	self.view.x2=self.view.x2+dx
	
	self.view.y1=self.view.y1+dy
	self.view.y2=self.view.y2+dy
	
	self.view.z=self.view.z+dz
	
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
function control_view.server:serve_view()
	local buf=self.buffer
	local w=self.view.width
	local h=self.view.height
	buf:reset()
	buf:append(messages.CONTROL_VIEW_UPDATE)
	buf:append(w,16)
	buf:append(h,16)
	local offBuf=offscreen.draw(self.view,self.view.z)
	appendScreen(offBuf,w,h,buf)
	print("sending screen:",w,h,w*h*7,#offBuf*7)
	buf:send(self.player.client)
end
--message parsing
function control_view.server:msg_set(buf)
	local x1,x2,y1,y2,z
	x1=buf:extract(nil,16)
	y1=buf:extract(nil,16)
	x2=buf:extract(nil,16)
	y2=buf:extract(nil,16)
	z=buf:extract(nil,16)
	local rect=gui.mkdims_xy(x1,y1,x2,y2)
	self:set_view(rect,z) 
	
	if not RESEND_ON_CHANGE then
		self:serve_view()
	end
end
function control_view.server:msg_update(buf)
	self:serve_view()
end
function control_view.server:msg_move(buf)
	local dx,dy,dz
	dx=buf:extract(nil,16)
	dy=buf:extract(nil,16)
	dz=buf:extract(nil,16)
	self:move_view(dx,dy,dz)
end

-- A client control
control_view.client.ATTRS={
	readyFunction=DEFAULT_NIL, -- a callback when the new screen is ready
	dirty=false,	--the screen got updated
}

function control_view.client:set_viewscreen(rect,z)
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
function control_view.client:move_viewscreen(dx,dy,dz)
	local buf=self.buffer
	buf:reset()
	buf:append(messages.CONTROL_VIEW_MOVE)
	buf:append(dx,16)
	buf:append(dy,16)
	buf:append(dz,16)
	buf:send()
end
function control_view.client:render()
	local buf=self.buffer
	buf:reset()
	buf:append(messages.CONTROL_VIEW_UPDATE)
	self.dirty=false
	buf:send()
end
--message parsing
function control_view.client:msg_update(buf)
	self.dirty=true
	local screen=self.screen or {}
	local w=buf:extract(nil,16)-1
	local h=buf:extract(nil,16)-1
	screen.w=w
	screen.h=h
	for x=0,w-1 do
	for y=0,h-1 do
		local idx=(x)*h+(y)
		screen[idx]={tile=buf:extract(nil,32),fg=buf:extract(),bg=buf:extract(),bold=buf:extract()}
	end
	end
	self.screen=screen
	if self.readyFunction then
		self.readyFunction(screen)
	end
end

return _ENV