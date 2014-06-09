local buffer=require('hack.scripts.multiplay.buffer').buffer
local args={...}
local socket=require "plugins.luasocket"

local HOST="localhost"
local PORT=1444

function server_tick(server,clients)

    local client=server:accept()

    if client then 
        print("New client")
        table.insert(clients,client)
        client.buf=buffer{client=client}
    end

    for _,v in ipairs(clients) do
        local m=v.buf
        m:append(42)
        m:append("Ping!")
        m:send()

        local r
        while not r do 
            r=m:receive()
        end
        print(m:extract())
        print(m:extract("string"))
    end
end
function client_tick( client )
    local m=client.buf
    
    if m:receive() then
        print(m:extract())
        print(m:extract("string"))
        m:append(44)
        m:append("pong!")
        m:send()
    end
   

end
if args[1]=="-s" then
    local server=socket.tcp:bind(HOST,PORT)
    local clients={}
    server:setTimeout(0,250)
    --for i=1,100000 do
    while true do
        server_tick(server,clients)
    end
    --end
    server:close()
else
   local client=socket.tcp:connect(HOST,PORT)
   client.buf=buffer{client=client}
   while true do
    client_tick(client)
   end   
end