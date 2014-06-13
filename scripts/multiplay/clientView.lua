--a simple view client for multiplay

local utils = require 'utils'
local gui = require 'gui'
local dlg = require 'gui.dialogs'
local widgets = require 'gui.widgets'

local clientBase=require('hack.scripts.multiplay.clientBase').client
local mods=require('hack.scripts.multiplay.modules')

local ViewControl=mods.require_client('control_view')

clientView=defclass(clientView,gui.FramedScreen)
clientView.ATTRS={
	frame_title = "Online Client",
	frame_background=nil
}
	
function clientView:init(args)
	self:addviews{
		--[[widgets.Panel{
			frame = { l = 0,b=8, r=20 },
			view_id='screen',
		},--]]
		widgets.Panel{
			frame = { b = 0 , h=1 },
			subviews={
				
				widgets.EditField{
					view_id='chat',
					
					}
				}
			}
	}
	self.view=ViewControl{readyFunction=self:callback("renderLater")} 
	self.base=clientBase{initControls={self.view}} 
	self.view.buffer=self.base.buffer
	self.base.onDoneInit=function()
		self.base:sendLogin("user","password")
		self.view:set_viewscreen(gui.mkdims_wh(0,0,20,20),4)
		self.view:render() -- start the rendering!
	end
	self.redraw=false
	self.base:sendVersion() -- a fix for chicken and egg problem
end
function clientView:renderLater( screen )
	self.redraw=true
	self.trg_screen=screen
end
--[[function clientView:onResize(w,h)
	self.view:set_viewscreen(gui.mkdims_wh(0,0,w,h),0)
end]]
function clientView:onRenderBody(dc)
	--if not self.redraw then
	--	return
	--end

	local sc=self.view.screen
	if sc==nil then
		return
	end

	local w=sc.w
	local h=sc.h
	for i=0,w do
	for j=0,h do
		local idx=(i)*h+(j)
		local t=sc[idx]
		if t then
			dc:seek(i+1,j+1):tile(t.tile or 0,0,t)
		end
	end
	end
	if self.redraw then
		self.redraw=false
		self.view:render() --ask for next frame
	end
end
function clientView:onIdle()
	self.base:tick()
end
function clientView:onInput(keys)
	if keys.STRING_A096 then -- '`'
		self:dismiss()
	elseif keys.A_MOVE_W then
		self.view:move_viewscreen(-1,0,0)
	elseif keys.A_MOVE_E then
		self.view:move_viewscreen(1,0,0)
	elseif keys.A_MOVE_N then
		self.view:move_viewscreen(0,-1,0)
	elseif keys.A_MOVE_S then
		self.view:move_viewscreen(0,1,0)
	elseif keys.A_MOVE_NW then
		self.view:move_viewscreen(-1,-1,0)
	elseif keys.A_MOVE_NE then
		self.view:move_viewscreen(1,-1,0)
	elseif keys.A_MOVE_SE then
		self.view:move_viewscreen(1,1,0)
	elseif keys.A_MOVE_SW then
		self.view:move_viewscreen(-1,1,0)
	elseif keys.A_MOVE_UP_AUX then
		self.view:move_viewscreen(0,0,1)
	elseif keys.A_MOVE_DOWN_AUX then
		self.view:move_viewscreen(0,0,-1)
	else
		if keys.SELECT then
			self.base.buffer:reset()
			self.base.buffer:append("Hello world")
			self.base.buffer:send()
		end
		--self:inputToSubviews(keys)
		--printall(keys)
	end
end
function clientView:onDismiss()
	self.base:shutdown()
end
local view=clientView{}
view:show()