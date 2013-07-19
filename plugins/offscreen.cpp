//an offscreen rendering thing. TODO: move into module.
#include <vector>

#include "PluginManager.h"
#include "LuaTools.h"
#include "modules/Offscreen.h"

#include "Types.h"
using df::coord2d;
using namespace DFHack;

DFHACK_PLUGIN("offscreen");

static void lua_pushTile(lua_State* L, const screenTile& t)
{
    lua_createtable(L,0,4);
    lua_pushnumber(L,t.tile);
    lua_setfield(L,-2,"tile");
    lua_pushnumber(L,t.fg);
    lua_setfield(L,-2,"fg");
    lua_pushnumber(L,t.bg);
    lua_setfield(L,-2,"bg");
    lua_pushboolean(L,t.bold);
    lua_setfield(L,-2,"bold");
}
static int lua_drawOffscreen(lua_State* L)
{
    Offscreen r; //todo do not create new one each time.
    int z;
    rect2d window;
    luaL_checktype(L,1,LUA_TTABLE);
    lua_getfield(L,1,"x1");
    window.first.x=lua_tonumber(L,-1);
    lua_pop(L,1);
    lua_getfield(L,1,"y1");
    window.first.y=lua_tonumber(L,-1);
    lua_pop(L,1);
    lua_getfield(L,1,"x2");
    window.second.x=lua_tonumber(L,-1);
    lua_pop(L,1);
    lua_getfield(L,1,"y2");
    window.second.y=lua_tonumber(L,-1);
    lua_pop(L,1);
    z=luaL_checknumber(L,2);
    
    std::vector<screenTile> buffer;
    r.drawBuffer(window,z,buffer); 

    lua_createtable(L,buffer.size(),0);
    for(size_t i=0;i<buffer.size();i++)
    {
        lua_pushnumber(L,i);
        lua_pushTile(L,buffer[i]);
        lua_settable(L,-3);
    }
    return 1;
}
DFHACK_PLUGIN_LUA_FUNCTIONS {
    
    DFHACK_LUA_END
};
DFHACK_PLUGIN_LUA_COMMANDS {
    {"draw",lua_drawOffscreen},
    DFHACK_LUA_END
};

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    
    return CR_OK;
}
