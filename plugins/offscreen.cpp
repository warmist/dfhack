//an offscreen rendering thing. TODO: move into module.
#include <vector>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "Types.h"
#include "TileTypes.h"
#include "LuaTools.h"
#include "Error.h"

#include "modules/MapCache.h"
#include "modules/Materials.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/building_drawbuffer.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "df/plant.h"

const int tilePics[]={32,31,247,30,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,240,0,0,0,0,84,88,62,60,0,0,0,0,178,0,86,35,88,62,60,88,62,60,42,43,43,43,43,43,0,88,62,60,88,62,60,88,62,60,88,62,60,88,62,60,0,206,0,15,0,0,39,0,0,0,0,0,
    0,0,0,79,79,79,79,79,0,0,0,0,0,247,247,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,176,177,178,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,178,0,0,0,0,0,0,0,0,0,0,0,83,0,30,30,30,30,30,30,30,30,30,242,243,126,30,0,0,0,0,0,0,0,0,0,0,0,0,
    178,178,178,126,178,176,126,15,206,197,0,214,213,
    212,211,190,189,184,183,206,204,203,202,185,201,200,188,187,186,205,214,213,212,211,190,189,184,183,206,204,203,202,185,201,200,188,187,186,205,214,213,212,211,190,189,184,183,206,204,203,202,185,201,200,188,187,186,205,206,206,176,177,178,219,176,177,178,219,39,44,96,46,39,44,96,46,39,44,96,46,39,
    44,96,46,39,44,96,46,39,44,96,46,206,176,177,178,
    219,247,247,247,247,247,247,247,247,236,236,236,236,236,236,236,236,236,0,0,0,0,0,39,44,96,46,84,83,86,39,44,96,46,39,44,96,46,236,0,0,39,44,96,46,39,44,96,46,39,44,96,46,214,213,212,211,190,189,184,
    183,206,204,203,202,185,201,200,188,187,186,205,206,176,177,178,178,39,44,96,46,0,39,44,96,46,214,213,212,211,190,189,184,183,206,204,203,202,185,201,200,188,187,186,205,30,30,30,30,30,30,30,30,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,43,206,79,214,213,212,211,190,189,184,183,206,204,203,202,185,201,200,188,187,186,205,88,62,60,30,208,210,198,181,186,200,188,201,187,205,204,185,202,203,206,208,210,198,181,186,200,188,201,187,205,204,185,202,203,206,208,210,198,181,186,200,188,201,187,205,204,185,202,203,206,208,210,198,181,186,200,188,201,187,205,204,185,202,203,206,208,210,198,181,186,200,188,201,187,205,204,185,202,203,206,208,210,198,181,186,200,188,201,187,205,204,185,202,203,206,30,30,30,30,30,
    30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
    30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
    30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,}; //dumped by a script :)
using df::coord2d;
using namespace DFHack;

DFHACK_PLUGIN("offscreen");


bool cache_invalid=true;
struct screenTile
{
    int tile;
    int8_t fg, bg;
    bool bold;
};
bool isInRect(const coord2d& pos,const rect2d& rect)
{
    if(pos.x>=rect.first.x && pos.y>=rect.first.y && pos.x<rect.second.x && pos.y<rect.second.y)
        return true;
    return false;
}
df::material* lookupMaterial(const t_matpair& mat,screenTile& trg,bool is_build=false)
{
    MaterialInfo nfo(mat);
    if(nfo.material)
    if(!is_build)
    {
        trg.fg=nfo.material->tile_color[0];
        trg.bg=nfo.material->tile_color[1];
        trg.bold=nfo.material->tile_color[2];
    }
    else
    {
        trg.fg=nfo.material->build_color[0];
        trg.bg=nfo.material->build_color[1];
        trg.bold=nfo.material->build_color[2];
    }
    return nfo.material;
}
void colorTile(const df::tiletype_material& tileMat,MapExtras::MapCache& cache,const DFCoord& pos,screenTile& trg,bool isUndug=false)
{
    t_matpair mat;
    
    switch(tileMat)
    {
    case df::tiletype_material::ASHES:
        trg.fg=COLOR_GREY;
        trg.bold=false;
        break;
    case df::tiletype_material::CAMPFIRE:
        trg.fg=COLOR_YELLOW;
        trg.bold=true;
        break;
    case df::tiletype_material::SOIL:
        mat=cache.baseMaterialAt(pos);
        {
            df::material* m=lookupMaterial(mat,trg);
            if(m && isUndug)
            {
                trg.tile=m->tile;
            }
            break;
        }
    case df::tiletype_material::STONE:
    case df::tiletype_material::LAVA_STONE:
        mat=cache.baseMaterialAt(pos);
        lookupMaterial(mat,trg);
        break;
    case df::tiletype_material::CONSTRUCTION:
        mat=cache.staticMaterialAt(pos);
        lookupMaterial(mat,trg,true);
        break;
    case df::tiletype_material::MINERAL:
        mat.mat_index=cache.veinMaterialAt(pos);
        mat.mat_type=0;//inorganic
        {
            df::material* m=lookupMaterial(mat,trg);
            if(m && isUndug)
            {
                trg.tile=m->tile;
            }
        }
        

        break;

    case df::tiletype_material::FROZEN_LIQUID:
        trg.fg=COLOR_CYAN;
        trg.bold=true;
        break;
    case df::tiletype_material::PLANT:
    case df::tiletype_material::GRASS_LIGHT: //MORE INFO IN MAP BLOCK EVENTS
        trg.fg=COLOR_GREEN;
        trg.bold=true;
        break;
    case df::tiletype_material::GRASS_DARK:
        trg.fg=COLOR_GREEN;
        trg.bold=false;
        break; 
    case df::tiletype_material::GRASS_DRY:
        trg.fg=COLOR_YELLOW;
        trg.bold=true;
        break;
    case df::tiletype_material::GRASS_DEAD:
        trg.fg=COLOR_BROWN;
        trg.bold=false;
        break;
    
    case df::tiletype_material::DRIFTWOOD:
        trg.fg=COLOR_WHITE;
        trg.bold=true;
        break;
    case df::tiletype_material::MAGMA:
        trg.fg=COLOR_RED;
        trg.bold=true;
        break;
    case df::tiletype_material::POOL:    
    case df::tiletype_material::RIVER:
    case df::tiletype_material::BROOK:
        trg.fg=COLOR_BROWN;
        trg.bold=false;
        break;
    }

}
void drawCreature(df::unit* u,screenTile& trg) //TODO cache this and then be fast!
{
    df::creature_raw* cr=df::creature_raw::find(u->race);
    if(cr)
    {
        if(u->caste<cr->caste.size())
        {
            df::caste_raw* casteRaw=cr->caste[u->caste];
            trg.tile=casteRaw->caste_tile;
            /*
            trg.fg=casteRaw->caste_color[0];
            trg.bg=casteRaw->caste_color[1];
            trg.bold=casteRaw->caste_color[2];
            */
            trg.fg=COLOR_WHITE;
            //TODO military, profesions and glowtiles
        }
    }
}
void drawPlant(df::plant* p,screenTile& trg)
{
    //TODO how to check if this is a dead plant??
    MaterialInfo mat(419, p->material);//TODO this should have some enum or const defined somewhere...
    if(!mat.plant)
    {
        trg.tile='?';
        trg.fg=COLOR_GREEN;
        trg.bold=true;
    }
    df::plant_raw* praw=mat.plant;
    if(p->grow_counter<180000) //also some constant?
    {
        //sapling
        trg.tile=praw->tiles.sapling_tile;
        trg.fg=praw->colors.sapling_color[0];
        trg.bg=praw->colors.sapling_color[1];
        trg.bold=praw->colors.sapling_color[2];
    }
    else
    {
        if(!p->flags.bits.is_shrub)
        {
            trg.tile=praw->tiles.tree_tile;
            trg.fg=praw->colors.tree_color[0];
            trg.bg=praw->colors.tree_color[1];
            trg.bold=praw->colors.tree_color[2];
        }
        else
        {
            trg.tile=praw->tiles.shrub_tile;
            trg.fg=praw->colors.shrub_color[0];
            trg.bg=praw->colors.shrub_color[1];
            trg.bold=praw->colors.shrub_color[2];
        }
    }
}
void drawItem(df::item* it,screenTile& trg) //BIG TODO
{
    trg.tile='?';
    trg.fg=COLOR_RED;
    trg.bold=true;
}
void drawOffscreen(const rect2d& window,int z,std::vector<screenTile>& buffer)
{
    if(!df::global::world)
        return;
    
    //TODO static array of images for each tiletype
    MapExtras::MapCache cache;
    int w=window.second.x-window.first.x;
    int h=window.second.y-window.first.y;
    rect2d localWindow=mkrect_wh(0,0,w,h);
    if(buffer.size()!=w*h)
        buffer.resize(w*h);
    //basic tiletype stuff here
    for(int x=window.first.x;x<window.second.x;x++) //todo, make it by block, minimal improvement over cache prob though
    for(int y=window.first.y;y<window.second.y;y++)
    {
        DFCoord coord(x,y,z);
        df::tiletype tt=cache.tiletypeAt(coord);
        df::tiletype_shape shape = ENUM_ATTR(tiletype,shape,tt);
        df::tiletype_shape_basic basic_shape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
        DFHack::t_matpair mat=cache.staticMaterialAt(coord);
        df::tiletype_material tileMat= ENUM_ATTR(tiletype,material,tt);
        df::tile_designation d=cache.designationAt(coord);
        df::tile_occupancy o=cache.occupancyAt(coord);
        df::tiletype_special sp=ENUM_ATTR(tiletype,special,tt);
        
        int wx=x-window.first.x;
        int wy=y-window.first.y;
        screenTile& curTile=buffer[wx*h+wy];
        if(d.bits.hidden)
        {
            curTile.tile=0;
            continue;
        }
        if(shape==df::tiletype_shape::EMPTY || shape==df::tiletype_shape::RAMP_TOP)
        {
            //empty,liquids and '.' for other stuff...
            DFCoord coord2(x,y,z-1);
            df::tiletype tt2=cache.tiletypeAt(coord2);
            df::tiletype_shape shape2 = ENUM_ATTR(tiletype,shape,tt);
            df::tiletype_material tileMat2= ENUM_ATTR(tiletype,material,tt2);
            df::tile_designation d2=cache.designationAt(coord2);
            df::tiletype_special sp2=ENUM_ATTR(tiletype,special,tt2);
            bool unDug2= (sp2!=df::tiletype_special::SMOOTH && shape2==df::tiletype_shape::WALL);

            if (d2.bits.flow_size>0)
            {
                if(shape!=df::tiletype_shape::RAMP_TOP) //don't show liquid amount on ramp tops
                    curTile.tile='0'+d2.bits.flow_size; //TODO lookup setting for this
                else
                    curTile.tile=tilePics[tt];
                curTile.fg=(d2.bits.liquid_type)?(COLOR_RED):(COLOR_BLUE);
                continue;
            }
            else if(shape2==df::tiletype_shape::EMPTY)
            {
                curTile.tile=178; //look up settings
                curTile.fg=COLOR_CYAN; 
                continue;
            }
            else
            {
                if(shape==df::tiletype_shape::RAMP_TOP)
                    curTile.tile=tilePics[tt];
                else
                    curTile.tile='.';
                colorTile(tileMat2,cache,coord2,curTile,unDug2);
                continue;
            }
        }
        bool inliquid=false;
        bool unDug= (sp!=df::tiletype_special::SMOOTH && shape==df::tiletype_shape::WALL);
        if (d.bits.flow_size>0)
        {
            curTile.tile='0'+d.bits.flow_size;
            curTile.fg=(d.bits.liquid_type)?(COLOR_RED):(COLOR_BLUE);
            curTile.bold=true;
            inliquid=true;
        }    
        if(!inliquid && shape!=df::tiletype_shape::RAMP_TOP)
        {
            curTile.tile=tilePics[tt];
            colorTile(tileMat,cache,coord,curTile,unDug);
            if(!unDug)
            {
                curTile.bg=0;
            }
        }
        else
        {
            if(shape==df::tiletype_shape::RAMP || shape==df::tiletype_shape::BROOK_BED || shape==df::tiletype_shape::RAMP_TOP)
                curTile.tile=tilePics[tt];
        }
        
    }
    //plants
    for(int bx=window.first.x/16;bx<=window.second.x/16;bx++) //blocks have items by id. So yeah each item a search would be slow
    for(int by=window.first.y/16;by<=window.second.y/16;by++)
    {
        MapExtras::Block* b=cache.BlockAt(DFCoord(bx,by,z));
        if(!b || !b->getRaw())
            continue;
        std::vector<df::plant*>& plants=b->getRaw()->plants;
        for(int i=0;i<plants.size();i++)
        {
            df::plant* p=plants[i];
            if(p->pos.z==z && isInRect(df::coord2d(p->pos.x,p->pos.y),window))
            {
                int wx=p->pos.x-window.first.x;
                int wy=p->pos.y-window.first.y;
                screenTile& curTile=buffer[wx*h+wy];
                drawPlant(p,curTile);
            }
        }
    }
    //in df items blink between stuff, but i don't have time for that
    std::vector<df::item*>& items=df::global::world->items.other[df::items_other_id::IN_PLAY];
    for(int i=0;i<items.size();i++)
    {
        df::item* it=items[i];
        if(it->flags.bits.on_ground && it->pos.z==z && isInRect(df::coord2d(it->pos.x,it->pos.y),window))
        {
            int wx=it->pos.x-window.first.x;
            int wy=it->pos.y-window.first.y;
            screenTile& curTile=buffer[wx*h+wy];
            drawItem(it,curTile);
        }
    }
    //buildings
    std::vector<df::building*>& buildings=df::global::world->buildings.all;
    for(int i=0;i<buildings.size();i++)
    {
        df::building* build=buildings[i];
        
        
        if(z!=build->z)
            continue;
        if(!build->isVisibleInUI())
            continue;
        if(isInRect(df::coord2d(build->x1,build->y1),window)||isInRect(df::coord2d(build->x2,build->y2),window))
        {
            df::building_drawbuffer drawBuffer;
            build->getDrawExtents(&drawBuffer);
            int bw=drawBuffer.x2-drawBuffer.x1;
            int bh=drawBuffer.y2-drawBuffer.y1;
            build->drawBuilding(&drawBuffer,0); //might be viewscreen dependant
            int wx=build->x1-window.first.x;
            int wy=build->y1-window.first.y;
            
            for(int x=0;x<=bw;x++)
            for(int y=0;y<=bh;y++)
            {
                df::coord2d p(x+wx,y+wy);
                if(isInRect(p,localWindow))
                {
                    screenTile& curTile=buffer[p.x*h+p.y];
                    if(drawBuffer.tile[x][y]!=32)
                    {
                        curTile.tile=drawBuffer.tile[x][y];
                        curTile.fg=drawBuffer.fore[x][y];
                        curTile.bg=drawBuffer.back[x][y];
                        curTile.bold=drawBuffer.bright[x][y];
                    }
                }
            }
        }
    }
    //units. TODO No multi tile units yet.
    std::vector<df::unit*>& units=df::global::world->units.active;
    for(int i=0;i<units.size();i++)
    {
        df::unit* u=units[i];
        if(!u->flags1.bits.dead && u->pos.z==z && isInRect(df::coord2d(u->pos.x,u->pos.y),window))
        {
            int wx=u->pos.x-window.first.x;
            int wy=u->pos.y-window.first.y;
            screenTile& curTile=buffer[wx*h+wy];
            drawCreature(u,curTile);
        }
    }
    //TODO blood and flows
}
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
    drawOffscreen(window,z,buffer);

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

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch(event)
    {
    case SC_WORLD_UNLOADED:
        cache_invalid=true;
        break;
    default:
        break;
    }
    return CR_OK;
}