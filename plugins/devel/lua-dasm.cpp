#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "Error.h"
#include "LuaTools.h"


#include "libdasm.h"

using namespace DFHack;

DFHACK_PLUGIN("lua-dasm");
#define SETFIELD_INSTR(n) Lua::SetField(L,instr-> n,-1,#n)
static int lua_get_instruction(lua_State*L)
{
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,1) || lua_isnil(L,1));
    CHECK_INVALID_ARGUMENT(lua_isnumber(L,2) || lua_islightuserdata(L,2))
    Mode m=MODE_32; //fixed because df is 32 bit.
    INSTRUCTION* instr;
    if(lua_isnil(L,1))
    {
        instr=new INSTRUCTION;
    }
    else
    {
        instr=(INSTRUCTION*)lua_touserdata(L,1);
    }
    BYTE* ptr;
    if(lua_isnumber(L,2))
    {
        ptr=(BYTE*)(size_t)lua_tonumber(L,2);
    }
    else
    {
        ptr=(BYTE*)lua_touserdata(L,2);
    }
    if(!get_instruction(instr,ptr,m))
    {
        //throw error?
    }
    lua_newtable(L);
    lua_pushlightuserdata(L,instr);
    lua_setfield(L,-2,"_ptr");
    SETFIELD_INSTR(length);	// Instruction length
    SETFIELD_INSTR(type);	// Instruction type
    SETFIELD_INSTR(mode);	// Addressing mode
    SETFIELD_INSTR(opcode);	// Actual opcode
    SETFIELD_INSTR(modrm);	// MODRM byte
    SETFIELD_INSTR(sib);	// SIB byte
    SETFIELD_INSTR(modrm_offset);	// MODRM byte offset
    SETFIELD_INSTR(extindex);	// Extension table index
    SETFIELD_INSTR(fpuindex);	// FPU table index
    SETFIELD_INSTR(dispbytes);	// Displacement bytes (0 = no displacement)
    SETFIELD_INSTR(immbytes);	// Immediate bytes (0 = no immediate)
    SETFIELD_INSTR(sectionbytes);	// Section prefix bytes (0 = no section prefix)
    SETFIELD_INSTR(flags);	// Instruction flags
    SETFIELD_INSTR(eflags_affected);	// Process eflags affected
    SETFIELD_INSTR(eflags_used);	// Processor eflags used by this instruction
    SETFIELD_INSTR(iop_written);	// mask of affected implied registers (written)
    SETFIELD_INSTR(iop_read);	// mask of affected implied registers (read)
    Lua::SetField(L,(size_t)ptr,-1,"offset");
    return 1;
}
/*(
    INSTRUCTION *inst,	// pointer to INSTRUCTION structure
    BYTE *addr,		// code buffer
    enum Mode mode		// mode: MODE_32 or MODE_16
);*/
#undef SETFIELD_INSTR
static int lua_get_instruction_string(lua_State*L)
{
    CHECK_INVALID_ARGUMENT(lua_istable(L,1));
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    INSTRUCTION* ptr=(INSTRUCTION*)lua_touserdata(L,-1);
    lua_getfield(L,1,"offset");
    size_t offset=lua_tonumber(L,-1);
    char buf[100];
    get_instruction_string(ptr,FORMAT_INTEL,offset,buf,100);
    lua_pushstring(L,buf);
    return 1;
}
/*int get_instruction_string(
    INSTRUCTION *inst,	// pointer to INSTRUCTION structure
    enum Format format,	// instruction format: FORMAT_ATT or FORMAT_INTEL
    DWORD offset,		// instruction absolute address
    char *string,		// string buffer
    int length		// string length
    );
*/
DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(lua_get_instruction),
    DFHACK_LUA_COMMAND(lua_get_instruction_string),
    DFHACK_LUA_END
};
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    return CR_OK;
}