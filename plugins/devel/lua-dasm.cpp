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
    CHECK_INVALID_ARGUMENT(lua_istable(L,1) || lua_isnil(L,1));
    CHECK_INVALID_ARGUMENT(lua_isnumber(L,2) || lua_islightuserdata(L,2))
    Mode m=MODE_32; //fixed because df is 32 bit.
    INSTRUCTION* instr;
    bool constructed=false;
    if(lua_isnil(L,1))
    {
        instr=new INSTRUCTION;
        constructed=true;
    }
    else
    {
        lua_getfield(L,1,"_ptr");
        CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
        instr=(INSTRUCTION*)lua_touserdata(L,-1);
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
        if(constructed)
            delete instr;
        return 0;
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
    if(get_instruction_string(ptr,FORMAT_INTEL,offset,buf,100))
    {
        lua_pushstring(L,buf);
        return 1;
    }
    else
    {
        return 0;
    }
}

/*int get_instruction_string(
    INSTRUCTION *inst,	// pointer to INSTRUCTION structure
    enum Format format,	// instruction format: FORMAT_ATT or FORMAT_INTEL
    DWORD offset,		// instruction absolute address
    char *string,		// string buffer
    int length		// string length
    );
*/
static int get_operand_string(lua_State*L)
{
    CHECK_INVALID_ARGUMENT(lua_istable(L,1));
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    INSTRUCTION* ptr=(INSTRUCTION*)lua_touserdata(L,-1);

    CHECK_INVALID_ARGUMENT(lua_isnumber(L,2)||lua_istable(L,2));
    POPERAND op;
    if(lua_isnumber(L,2))
    {
        int op_num=lua_tonumber(L,2);
        switch(op_num)
        {
        case 0:
            op=&ptr->op1;
            break;
        case 1:
            op=&ptr->op2;
            break;
        case 2:
            op=&ptr->op3;
            break;
        default:
            luaL_error(L,"Invalid operator number");
        }
    }
    else//lightuserdata
    {
        lua_getfield(L,1,"_ptr");
        CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
        op=(POPERAND)lua_touserdata(L,-1);
    }
    
    lua_getfield(L,1,"offset");
    size_t offset=lua_tonumber(L,-1);
    char buf[100];
    
    if(get_operand_string(ptr,op,FORMAT_INTEL,offset,buf,100))
    {
       lua_pushstring(L,buf);
        return 1;
    }
    else
        return 0;
}

/*
int get_operand_string(
    INSTRUCTION *inst,	// pointer to INSTRUCTION structure
    POPERAND op,		// pointer to OPERAND structure
    enum Format format,	// instruction format: FORMAT_ATT or FORMAT_INTEL
    DWORD offset,		// instruction absolute address
    char *string,		// string buffer
    int length		// string length
    );*/
#define SETFIELD_OP(n) Lua::SetField(L,op-> n,-1,#n)
void push_operand(lua_State*L,POPERAND op)
{
    lua_newtable(L);
    lua_pushlightuserdata(L,op);
    lua_setfield(L,-2,"_ptr");
    SETFIELD_OP(type);	  // Operand type (register, memory, etc)
    SETFIELD_OP(reg);	        // Register (if any)
    SETFIELD_OP(basereg);	        // Base register (if any)
    SETFIELD_OP(indexreg);	       // Index register (if any)
    SETFIELD_OP(scale);	      // Scale (if any)
    SETFIELD_OP(dispbytes);	      // Displacement bytes (0 = no displacement)
    SETFIELD_OP(dispoffset);	     // Displacement value offset
    SETFIELD_OP(immbytes);	       // Immediate bytes (0 = no immediate)
    SETFIELD_OP(immoffset);	      // Immediate value offset
    SETFIELD_OP(sectionbytes);	   // Section prefix bytes (0 = no section prefix)
    SETFIELD_OP(section);	       // Section prefix value
    SETFIELD_OP(displacement);	 // Displacement value
    SETFIELD_OP(immediate);	    // Immediate value
    SETFIELD_OP(flags);	      // Operand flags
}
#undef  SETFIELD_OP
static int lua_get_source_operand(lua_State*L)
{
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    PINSTRUCTION instr=(INSTRUCTION*)lua_touserdata(L,-1);
    POPERAND op=get_source_operand(instr);
    if(op)
    {
        push_operand(L,op);
        return 1;
    }
    else
        return 0;
}
static int lua_get_destination_operand(lua_State*L)
{
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    PINSTRUCTION instr=(INSTRUCTION*)lua_touserdata(L,-1);
    POPERAND op=get_destination_operand(instr);
    if(op)
    {
        push_operand(L,op);
        return 1;
    }
    else
        return 0;
}
/*POPERAND get_source_operand(
    PINSTRUCTION inst
    );
POPERAND get_destination_operand(
    PINSTRUCTION inst
    );
*/
static int lua_get_operand_immediate(lua_State*L)
{
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    POPERAND op=(POPERAND)lua_touserdata(L,-1);
    DWORD ret=0;
    if(get_operand_immediate(op,&ret))
    {
        lua_pushnumber(L,ret);
        return 1;
    }
    else
        return 0;
}
static int lua_get_operand_displacement(lua_State*L)
{
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    POPERAND op=(POPERAND)lua_touserdata(L,-1);
    DWORD ret=0;
    if(get_operand_displacement(op,&ret))
    {
        lua_pushnumber(L,ret);
        return 1;
    }
    else
        return 0;
}
/*int get_operand_immediate(
    POPERAND op,
    DWORD *imm		// returned immediate value
    );
int get_operand_displacement(
    POPERAND op,
    DWORD *disp		// returned displacement value
    );*/
static int lua_get_mnemonic_string(lua_State*L)
{
    CHECK_INVALID_ARGUMENT(lua_istable(L,1));
    lua_getfield(L,1,"_ptr");
    CHECK_INVALID_ARGUMENT(lua_islightuserdata(L,-1));
    INSTRUCTION* ptr=(INSTRUCTION*)lua_touserdata(L,-1);
    char buf[100];
    if(get_mnemonic_string(ptr,FORMAT_INTEL,buf,100))
    {
        lua_pushstring(L,buf);
        return 1;
    }
    else
    {
        return 0;
    }
}
/*int get_mnemonic_string(
    INSTRUCTION *inst,	// pointer to INSTRUCTION structure
    enum Format format,	// instruction format: FORMAT_ATT or FORMAT_INTEL
    char *string,		// string buffer
    int length		// string length
    );*/
DFHACK_PLUGIN_LUA_COMMANDS{
    {"get_instruction",lua_get_instruction},
    {"get_instruction_string",lua_get_instruction_string},
    {"get_operand_string",lua_get_instruction_string},
    {"get_source_operand",lua_get_source_operand},
    {"get_destination_operand",lua_get_destination_operand},
    {"get_operand_immediate",lua_get_operand_immediate},
    {"get_mnemonic_string",lua_get_mnemonic_string},
    DFHACK_LUA_END
};
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    return CR_OK;
}