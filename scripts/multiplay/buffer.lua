-- a buffer for sending/recieving stuff
local _ENV = mkmodule('hack.scripts.multiplay.buffer')
local socket=require("plugins.luasocket")
-- TODO nul terminated strings is good or not?
buffer=defclass(buffer)
buffer.ATTRS={
	msgBuf=DEFAULT_NIL,
	recBuf=DEFAULT_NIL,
	bufferSize=255, --or maxSize
	offset=0,
	readOffset=0
}
function buffer:init(args)
	self.msgBuf=df.new("uint8_t",self.bufferSize)
	setmetatable(self,{
		__gc=function(obj)
			print("GC collected:"..obj.bufferSize)
			df.delete(obj.msgBuf)
		end,__index=buffer})
end
function buffer:splitInt16(number,tbl,offset)
	tbl=tbl or {}
	offset=offset or 0
	tbl[offset]=bit32.band(bit32.arshift(number,8),255)
	tbl[offset+1]=bit32.band(number,255)
	return tbl
end
function buffer:splitInt32(number,tbl,offset)
	tbl=tbl or {}
	offset=offset or 0
	tbl[offset]=bit32.band(bit32.arshift(number,24),255)
	tbl[offset+1]=bit32.band(bit32.arshift(number,16),255)
	tbl[offset+2]=bit32.band(bit32.arshift(number,8),255)
	tbl[offset+3]=bit32.band(number,255)
	return tbl
end
function buffer:collectInt16(tbl,offset)
	offset=offset or 0
	return bit32.lshift(tbl[offset],8)+
		   tbl[offset+1]
end
function buffer:collectInt32(tbl,offset)
	offset=offset or 0
	return bit32.lshift(tbl[offset],24)+
		bit32.lshift(tbl[offset+1],16)+
		bit32.lshift(tbl[offset+2],8)+
		tbl[offset+3]
end

function buffer:toString(data,offset,size,maxSize)
	local tbl={}
	maxSize=maxSize or self.bufferSize
	offset=offset or 0
	if size==nil then
		for i=offset,maxSize-1 do
			if data[i] ~= 0 then
				table.insert(tbl,data[i])
			else
				break
			end
		end
	else
		for i=offset,size-1 do
			table.insert(tbl,data[i])
		end
	end
	return string.char(table.unpack(tbl)) --bad idea for long strings
end
function buffer:insertStr(str,offset)
	offset=offset or 0
	for i=0,#str-1 do
		self.msgBuf[i+offset]= string.byte(str,i+1)
	end
	self.msgBuf[#str+offset]=0
	return #str+offset+1
end
function buffer:append(data,intSize)
	if type(data)=='string' then
		self.offset=self:insertStr(data,self.offset)
	elseif type(data)=='number' and (intSize==nil or intSize==8) then
		self.msgBuf[self.offset]=data
		self.offset=self.offset+1
	elseif type(data)=='number' and intSize==16 then
		self:splitInt16(data,self.msgBuf,self.offset)
		self.offset=self.offset+2
	elseif type(data)=='number' and intSize==32 then
		self:splitInt32(data,self.msgBuf,self.offset)
		self.offset=self.offset+4
	else
		error("unsupported type")
	end
end
function buffer:extract(dataType,intSize,peek)
	peek=peek or false
	dataType=dataType or 'number'
	if dataType=='string' then
		local retString=self:toString(self.recBuf,self.readOffset)
		if not peek then
			self.readOffset=self.readOffset+#retString+1
		end
		return retString
	elseif dataType=='number' and (intSize==nil or intSize==8) then
		local retNum=self.recBuf[self.readOffset]
		if not peek then
			self.readOffset=self.readOffset+1
		end
		return retNum
	elseif dataType=='number' and intSize==16 then
		local retNum=self:collectInt16(self.recBuf,self.readOffset)
		if not peek then
			self.readOffset=self.readOffset+2
		end
		return retNum
	elseif dataType=='number' and intSize==32 then
		local retNum=self:collectInt32(self.recBuf,self.readOffset)
		if not peek then
			self.readOffset=self.readOffset+4
		end
		return retNum
	else
		error("unsupported type")
	end
end
function buffer:reset()
	print("BUFFER| reset",self.offset,self.readOffset)
	self.offset=0
	self.readOffset=0
end
function buffer:receive(data,size)
	self:reset()
	print("BUFFER| received:",size)
	for i=0,size-1 do
		print(data[i])
	end
	self.recBuf=data
	self.readOffset=0
end
function buffer:send(id)
	print("BUFFER| sending:",self.offset)
	socket.lua_sock_send(id,self.offset,self.msgBuf)
	self:reset()
end

return _ENV