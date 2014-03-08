--a simple view client for multiplay

local utils = require 'utils'
local gui = require 'gui'
local dlg = require 'gui.dialogs'
local widgets = require 'gui.widgets'

local clientBase=require('hack.scripts.multiplay.clientBase').client
local ViewControl=require('hack.scripts.multiplay.control_view_free').control_view.client

clientView=defclass(clientView,gui.FramedScreen)
clientView.ATTRS={
	frame_title = "Online Client",
}
	
function clientView:init(args)
	self:addviews{
		widgets.Panel{
			frame = { l = 0,b=8, r=20 },
			view_id='screen',
		},
		widgets.Panel{
			frame = { b = 0 , h=8 },
			subviews={
				
				widgets.EditField{
					view_id='chat',
					
					}
				}
			}
	}
	self.base=clientBase{}
	self.view=ViewControl{buffer=self.base.buffer}
	self.base:addControl(self.view)
	self.base.onDoneInit=function()
		self.base:sendLogin("user","password")
		self.view:set_viewscreen(gui.mkdims_wh(0,0,20,20),0)
		self.view:render() -- start the rendering!
	end
end
--[[function clientView:onResize(w,h)
	self.view:set_viewscreen(gui.mkdims_wh(0,0,w,h),0)
end]]
function clientView:onRenderBody(painter)
	if not self.base.dirty then
		return
	end
	local sc=self.base.screen
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
	self.base.dirty=false
	self.view:render() --ask for next frame
end
function clientView:onInput(keys)
	if keys.STRING_A096 then -- '`'
		self:dismiss()
	else
		--self:inputToSubviews(keys)
		printall(keys)
	end
end
function clientView:onDismiss()
	self.base:shutdown()
end
local view=clientView{}
view:show()