local dis = require 'plugins.lua-dasm'
function get_df_memory()
    local mem_table
    for k,v in pairs(dfhack.internal.getMemRanges()) do
        if v.execute then
            --print(v.name)
            mem_table=v
            break
        end
    end
    --printall(mem_table)
    return mem_table    
end
function cache_offsets()
    local cache={}
    for k,v in pairs(df.global) do
        cache[dfhack.internal.getAddress(k)]=k
    end
    --[[for k,v in pairs(cache) do
        print(v,string.format("%08x",k))
    end]]
    return cache
end
local add_cache=cache_offsets()
local counts={}
local dfmem=get_df_memory()
local call_table={}
local start=dfmem.start_addr
local off=0
local i1=dis.get_instruction(nil,start)
local context = 0

for i=1,20000 do
    local mn=dis.get_mnemonic_string(i1)
    if  mn== "mov" and  context==0 then
        local is=dis.get_instruction_string(i1)
        
        local source=dis.get_source_operand(i1)
        if source then --and source.type==1 and source.basereg==8 then
            local imm=dis.get_operand_immediate(source)
            if imm and add_cache[imm] then
                print("====================")
                
                local name=add_cache[imm]
                print(string.format("%08x %s --> %s",start+off,is,name))
                
                if counts[name] then counts[name]=counts[name]+1 else counts[name]=1 end
                context=10
            end
            --print(string.format("%08x",imm))
        end
        --table.insert(call_table,{off,is})
    elseif context>0 then
        local is=dis.get_instruction_string(i1)
        print(string.format("%08x %s",start+off,is))
        context=context-1
    end
    off=off+i1.length
    if off>= dfmem.end_addr then
        print("Reached end!")
        break
    end
    i1=dis.get_instruction(i1,start+off)
    if i1 ==nil then
        print(string.format("Failed @%08x",start+off))
        --break
        off=off+1
        i1=dis.get_instruction(i1,start+off)
    end
end
printall(counts)