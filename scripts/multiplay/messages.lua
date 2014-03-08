local _ENV = mkmodule('hack.scripts.multiplay.messages')
--TODO idea: add some structure to each message, and then easily serialize/deserialize any message
PROTOCOL_VERSION=1

MSG_TYPES={
	--control view
	"VIEWSCREEN_SET", -- c->s x1,y1,x2,y2,z  
	"VIEWSCREEN_UPDATE",-- s->c  w,h,data[w*h*tileSize] |c->s 
	"VIEWSCREEN_MOVE", --  c->s dx,dy,dz
	--misc
	"LOGIN", -- c->s username,password
	"CHAT",
	"INFO","ERROR","ERROR_FATAL", -- message ! could be used both s->c and c->s. Fatal version disconnects
	
	"VERSION"-- s->c protocol ver,No of supported controls, list of { control ID } | c->s protocol ver,No of supported controls, list of { control ID }
	--client should respond to VERSION with his VERSION
	}
for k,v in ipairs(MSG_TYPES) do
	MSG_TYPES[v]=k
end

return _ENV