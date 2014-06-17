-- a buffer for sending/recieving stuff
local _ENV = mkmodule('hack.scripts.multiplay.buffer')
local socket=require("plugins.luasocket")
-- TODO nul terminated strings is good or not?

buffer=defclass(buffer)
buffer.ATTRS={
	msgBuf=DEFAULT_NIL,
	recBuf=DEFAULT_NIL,
	bufferSize=3000, --or maxSize
	offset=0, --non 0 because of buffer size prefix --TODO fix string mode!
	readOffset=0,
	client=DEFAULT_NIL,
	mode=DEFAULT_NIL
}
function buffer:init(args)
	if self.msgBuf==nil then
		self.msgBuf=df.new("uint8_t",self.bufferSize)
		setmetatable(self,{
			__gc=function(obj)
				print("GC collected:"..obj.bufferSize)
				df.delete(obj.msgBuf)
			end,__index=buffer})
	end
	self:reset()
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
		if self.offset+#data>=self.bufferSize then
			error("buffer overflow")
		end
		self.offset=self:insertStr(data,self.offset)
	elseif type(data)=='number' and (intSize==nil or intSize==8) then
		if self.offset+1>=self.bufferSize then
			error("buffer overflow")
		end
		self.msgBuf[self.offset]=data
		self.offset=self.offset+1
	elseif type(data)=='number' and intSize==16 then
		if self.offset+2>=self.bufferSize then
			error("buffer overflow")
		end
		self:splitInt16(data,self.msgBuf,self.offset)
		self.offset=self.offset+2
	elseif type(data)=='number' and intSize==32 then
		if self.offset+4>=self.bufferSize then
			error("buffer overflow")
		end
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
function buffer:reset(stringmode)
	--print("BUFFER| reset",self.offset,self.readOffset)
	self.offset=0
	self.readOffset=0
	if not stringmode then
		self:append(0,32)
	end
end
function buffer:receive(client,stringmode) --can be improved with better buffers!
	self:reset(stringmode)
	client=client or self.client
	stringmode=stringmode or self.mode
	if stringmode then
		local data={}
		local s=client:receive()
		--print("receive:",s)
		for i=1,#s do
			data[i-1]=string.byte(s,i)
		end
		self.recBuf=data
		self.readOffset=0
	else
		local s=client:receive(4) -- get buf size
		if s==nil then
			return
		end
		local size=self:collectInt32({s:byte(1,4)},1)
		--print("receive header",size)
		if size<=0 then
			error("invalid packet size")
		end
		local ss=self.client:receive(size)
		if ss==nil then
			error("partial read")
		end
		
		--print("received body:")
		self.recBuf=self.recBuf or {}
		for i=1,#ss do
			self.recBuf[i-1]=string.byte(ss,i)
		end
		self.readOffset=0
	end
	return #self.recBuf
end
function buffer:size( )
	return #self.recBuf,self.offset
end
function buffer:send(client)
	--print("BUFFER| sending:",self.offset)
	client=client or self.client
	--self.msgBuf[0]=self.offset-1
	self:splitInt32(self.offset-4,self.msgBuf)
	client:send(self.msgBuf,self.offset)
	self:reset()
end

return _ENV