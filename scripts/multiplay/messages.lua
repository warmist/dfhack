local _ENV = mkmodule('hack.scripts.multiplay.messages')
--TODO idea: add some structure to each message, and then easily serialize/deserialize any message
PROTOCOL_VERSION=1

MSG_TYPES={
	
	--misc
	"LOGIN", -- c->s username,password
	"CHAT",
	"INFO","ERROR","ERROR_FATAL", -- message ! could be used both s->c and c->s. Fatal version disconnects
	
	"VERSION",-- s->c protocol ver,No of supported controls, list of { control ID } | c->s protocol ver,No of supported controls, list of { control ID }
	--client should respond to VERSION with his VERSION

	--control view
	"CONTROL_VIEW_SET", -- c->s x1,y1,x2,y2,z  
	"CONTROL_VIEW_UPDATE",-- s->c  w,h,data[w*h*tileSize] |c->s 
	"CONTROL_VIEW_MOVE", --  c->s dx,dy,dz

	--dwarf watch
	"DWARF_WATCH_NEW_DWARF", -- c->s ask for new dwarf, s->c new dwarf issued: id(32),fullname(string)
	"DWARF_WATCH_FOLLOW",	-- c->s enable(8): makes "control view" center on your dwarf (enable or disable)
	"DWARF_WATCH_DWARF_INFO", -- c->s message(string) misc dwarf message
	}
for k,v in ipairs(MSG_TYPES) do
	MSG_TYPES[v]=k
end

return _ENV