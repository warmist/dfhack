#include "map_reading.hpp"

#include "LuaTools.h"

#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Screen.h"
#include "modules/Units.h"

#include "df/block_square_event_material_spatterst.h"
#include "df/building.h"
#include "df/building_doorst.h"
#include "df/building_floodgatest.h"
#include "df/flow_info.h"
#include "df/graphic.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "df/plant.h"
#include "df/plant_raw.h"
#include "df/unit.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

using df::global::gps;
using namespace DFHack;
using df::coord2d;

rgbf lua_parseLightCell(lua_State* L)
{
    rgbf ret;

    lua_pushnumber(L,1);
    lua_gettable(L,-2);
    ret.r=lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_pushnumber(L,2);
    lua_gettable(L,-2);
    ret.g=lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_pushnumber(L,3);
    lua_gettable(L,-2);
    ret.b=lua_tonumber(L,-1);
    lua_pop(L,1);

    //Lua::GetOutput(L)->print("got cell(%f,%f,%f)\n",ret.r,ret.g,ret.b);
    return ret;
}
#define GETLUAFLAG(field,name) lua_getfield(L,-1,"flags");\
    if(lua_isnil(L,-1)){field=false;}\
    else{lua_getfield(L,-1,#name);field=lua_isnil(L,-1);lua_pop(L,1);}\
    lua_pop(L,1)

#define GETLUANUMBER(field,name) lua_getfield(L,-1,#name);\
    if(!lua_isnil(L,-1) && lua_isnumber(L,-1))field=lua_tonumber(L,-1);\
    lua_pop(L,1)

#define GETLUASTRING(field,name) lua_getfield(L,-1,#name);\
    if(!lua_isnil(L,-1) && lua_isstring(L,-1))field=lua_tostring(L,-1);\
    lua_pop(L,1)
matLightDef lua_parseMatDef(lua_State* L)
{

    matLightDef ret;
    lua_getfield(L,-1,"tr");
    if(ret.isTransparent=!lua_isnil(L,-1))
    {
        ret.transparency=lua_parseLightCell(L);
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"em");
    if(ret.isEmiting=!lua_isnil(L,-1))
    {
        ret.emitColor=lua_parseLightCell(L);
        lua_pop(L,1);
        lua_getfield(L,-1,"rad");
        if(lua_isnil(L,-1))
        {
            lua_pop(L,1);
            luaL_error(L,"Material has emittance but no radius");
        }
        else
            ret.radius=lua_tonumber(L,-1);
        lua_pop(L,1);
    }
    else
        lua_pop(L,1);
    GETLUAFLAG(ret.flicker,"flicker");
    return ret;
}
int light_config::parseMaterials(lua_State* L)
{
    auto engine= (light_config*)lua_touserdata(L, 1);
    engine->matDefs.clear();
    //color_ostream* os=Lua::GetOutput(L);
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"materials");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Materials table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        int type=lua_tonumber(L,-2);
        //os->print("Processing type:%d\n",type);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int index=lua_tonumber(L,-2);
            //os->print("\tProcessing index:%d\n",index);
            engine->matDefs[std::make_pair(type,index)]=lua_parseMatDef(L);

            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    return 0;
}
#define LOAD_SPECIAL(lua_name,class_name) \
    lua_getfield(L,-1,#lua_name);\
    if(!lua_isnil(L,-1))engine->class_name=lua_parseMatDef(L);\
    lua_pop(L,1)
int light_config::parseSpecial(lua_State* L)
{
    auto engine= (light_config*)lua_touserdata(L, 1);
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"special");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Special table not found.");
        return 0;
    }
    LOAD_SPECIAL(LAVA,matLava);
    LOAD_SPECIAL(WATER,matWater);
    LOAD_SPECIAL(FROZEN_LIQUID,matIce);
    LOAD_SPECIAL(AMBIENT,matAmbience);
    LOAD_SPECIAL(CURSOR,matCursor);
    LOAD_SPECIAL(CITIZEN,matCitizen);
    GETLUANUMBER(engine->levelDim,levelDim);
    GETLUANUMBER(engine->dayHour,dayHour);
    GETLUANUMBER(engine->daySpeed,daySpeed);
    GETLUANUMBER(engine->num_bounces,bounceCount);
    GETLUANUMBER(engine->adv_mode, advMode);
    GETLUANUMBER(engine->max_radius, maxRadius);
    GETLUANUMBER(engine->adapt_speed, adaptSpeed);
    GETLUANUMBER(engine->adapt_brightness, adaptBrightness);
    GETLUASTRING(engine->opencl_program, openclProgram);
    lua_getfield(L,-1,"dayColors");
    if(lua_istable(L,-1))
    {
        engine->dayColors.clear();
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            engine->dayColors.push_back(lua_parseLightCell(L));
            lua_pop(L,1);
        }
        lua_pop(L,1);
    }
    return 0;
}
#undef LOAD_SPECIAL
int light_config::parseItems(lua_State* L)
{
    auto engine= (light_config*)lua_touserdata(L, 1);
    engine->itemDefs.clear();
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"items");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Items table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if(!lua_istable(L,-1))
            luaL_error(L,"Broken item definitions.");
        lua_getfield(L,-1,"type");
        int type=lua_tonumber(L,-1);
        lua_pop(L,1);
        lua_getfield(L,-1,"subtype");
        int subtype=luaL_optinteger(L,-1,-1);
        lua_pop(L,1);
        itemLightDef item;
        lua_getfield(L,-1,"light");
        item.light=lua_parseMatDef(L);
        GETLUAFLAG(item.haul,"hauling");
        GETLUAFLAG(item.equiped,"equiped");
        GETLUAFLAG(item.inBuilding,"inBuilding");
        GETLUAFLAG(item.inContainer,"contained");
        GETLUAFLAG(item.onGround,"onGround");
        GETLUAFLAG(item.useMaterial,"useMaterial");
        engine->itemDefs[std::make_pair(type,subtype)]=item;
        lua_pop(L,2);
    }
    lua_pop(L,1);
    return 0;
}
int light_config::parseCreatures(lua_State* L)
{
    auto engine= (light_config*)lua_touserdata(L, 1);
    engine->creatureDefs.clear();
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"creatures");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Creatures table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if(!lua_istable(L,-1))
            luaL_error(L,"Broken creature definitions.");
        lua_getfield(L,-1,"race");
        int race=lua_tonumber(L,-1);
        lua_pop(L,1);
        lua_getfield(L,-1,"caste");
        int caste=lua_tonumber(L,-1);
        lua_pop(L,1);
        creatureLightDef cr;
        lua_getfield(L,-1,"light");
        cr.light=lua_parseMatDef(L);
        engine->creatureDefs[std::make_pair(race,caste)]=cr;
        lua_pop(L,2);
    }
    lua_pop(L,1);
    return 0;
}
int light_config::parseBuildings(lua_State* L)
{
    auto engine= (light_config*)lua_touserdata(L, 1);
    engine->buildingDefs.clear();
    Lua::StackUnwinder unwinder(L);
    lua_getfield(L,2,"buildings");
    if(!lua_istable(L,-1))
    {
        luaL_error(L,"Buildings table not found.");
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        int type=lua_tonumber(L,-2);
        if(!lua_istable(L,-1))
        {
            luaL_error(L,"Broken building definitions.");
        }
        //os->print("Processing type:%d\n",type);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int subtype=lua_tonumber(L,-2);
            //os->print("\tProcessing subtype:%d\n",index);
            lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            int custom=lua_tonumber(L,-2);
            //os->print("\tProcessing custom:%d\n",index);
            buildingLightDef current;
            current.light=lua_parseMatDef(L);
            engine->buildingDefs[std::make_tuple(type,subtype,custom)]=current;
            GETLUAFLAG(current.poweredOnly,"poweredOnly");
            GETLUAFLAG(current.useMaterial,"useMaterial");

            lua_getfield(L,-1,"size");
            current.size=luaL_optnumber(L,-1,1);
            lua_pop(L,1);

            lua_getfield(L,-1,"thickness");
            current.thickness=luaL_optnumber(L,-1,1);
            lua_pop(L,1);

            lua_pop(L, 1);
        }

            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L,1);
    return 0;
}
void light_config::defaultSettings()
{
    matAmbience=matLightDef(rgbf(0.85f,0.85f,0.85f));
    matLava=matLightDef(rgbf(0.8f,0.2f,0.2f),rgbf(0.8f,0.2f,0.2f),5);
    matWater=matLightDef(rgbf(0.6f,0.6f,0.8f));
    matIce=matLightDef(rgbf(0.7f,0.7f,0.9f));
    matCursor=matLightDef(rgbf(0.96f,0.84f,0.03f),11);
    matCursor.flicker=true;
    matWall=matLightDef(rgbf(0,0,0));
    matCitizen=matLightDef(rgbf(0.8f,0.8f,0.9f),6);
    levelDim=0.002f;
    dayHour=-1;
    daySpeed=1;
    adv_mode = 0;
    num_bounces = 0;
    dayColors.push_back(rgbf(0,0,0));
    dayColors.push_back(rgbf(1,1,1));
    dayColors.push_back(rgbf(0,0,0));
    max_radius = 15;
    adapt_speed = 0.1;
    adapt_brightness = 100;
    opencl_program = R"(
__kernel void calculate_light(__global __read_only float4* occlusion,__global __read_only float4* lights,__global __write_only float4* output ){}
__kernel void convert_color(__global float4* lights ){}
)";
}
void light_config::load_settings()
{
    std::string rawFolder;
    std::string save_dir = df::global::world->cur_savegame.save_dir;
    if(save_dir!="" && save_dir!="arena1" )
    {
        rawFolder= "data/save/" + save_dir + "/raw/";
    }
    else
    {
        rawFolder= "raw/";
    }
    const std::string settingsfile=rawFolder+"rendermax.lua";

    CoreSuspender lock;
    color_ostream_proxy out(Core::getInstance().getConsole());

    lua_State* s=DFHack::Lua::Core::State;
    lua_newtable(s);
    int env=lua_gettop(s);
    try{
        int ret=luaL_loadfile(s,settingsfile.c_str());
        if(ret==LUA_ERRFILE)
        {
            out.printerr("File not found:%s\n",settingsfile.c_str());
            lua_pop(s,1);
            ret = luaL_loadfile(s, "raw/rendermax.lua");
            if (ret == LUA_ERRFILE)
            {
                lua_pop(s, 1);
                out.printerr("Fallback file (raw/rendermax.lua) not found\n");
                return;
            }
        }

        if(ret==LUA_ERRSYNTAX)
        {
            out.printerr("Syntax error:\n\t%s\n",lua_tostring(s,-1));
        }
        else
        {
            lua_pushvalue(s,env);

            if(Lua::SafeCall(out,s,1,0))
            {
                lua_pushcfunction(s, parseMaterials);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%d materials loaded\n",matDefs.size());

                lua_pushcfunction(s, parseSpecial);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%d day light colors loaded\n",dayColors.size());

                lua_pushcfunction(s, parseBuildings);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%d buildings loaded\n",buildingDefs.size());

                lua_pushcfunction(s, parseCreatures);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%d creatures loaded\n",creatureDefs.size());

                lua_pushcfunction(s, parseItems);
                lua_pushlightuserdata(s, this);
                lua_pushvalue(s,env);
                Lua::SafeCall(out,s,2,0);
                out.print("%d items loaded\n",itemDefs.size());
            }

        }
    }
    catch(std::exception& e)
    {
        out.printerr("%s",e.what());
    }
    lua_pop(s,1);
}
#undef GETLUAFLAG
#undef GETLUANUMBER
const matLightDef* light_config::getMaterialDef(int matType, int matIndex) const
{
    auto it = matDefs.find(std::make_pair(matType, matIndex));
    if (it != matDefs.end())
        return &it->second;
    else
        return NULL;
}
const buildingLightDef* light_config::getBuildingDef(df::building* bld) const
{
    auto it = buildingDefs.find(std::make_tuple((int)bld->getType(), (int)bld->getSubtype(), (int)bld->getCustomType()));
    if (it != buildingDefs.end())
        return &it->second;
    else
        return NULL;
}
const creatureLightDef* light_config::getCreatureDef(df::unit* u) const
{
    auto it = creatureDefs.find(std::make_pair(int(u->race), int(u->caste)));
    if (it != creatureDefs.end())
        return &it->second;
    else
    {
        auto it2 = creatureDefs.find(std::make_pair(int(u->race), int(-1)));
        if (it2 != creatureDefs.end())
            return &it2->second;
        else
            return NULL;
    }
}
const itemLightDef* light_config::getItemDef(df::item* it) const
{
    auto iter = itemDefs.find(std::make_pair(int(it->getType()), int(it->getSubtype())));
    if (iter != itemDefs.end())
        return &iter->second;
    else
    {
        auto iter2 = itemDefs.find(std::make_pair(int(it->getType()), int(-1)));
        if (iter2 != itemDefs.end())
            return &iter2->second;
        else
            return NULL;
    }
}
rgbf light_config::get_sky_color(float v) const
{
    if (dayColors.size()<2)
    {
        v = abs(fmod(v + 0.5, 1) - 0.5) * 2;
        return rgbf(v, v, v);
    }
    else
    {
        float pos = v*(dayColors.size() - 1);
        int pre = floor(pos);
        pos -= pre;
        if (pre == dayColors.size() - 1)
            return dayColors[pre];
        return dayColors[pre] * (1 - pos) + dayColors[pre + 1] * pos;
    }
}
void apply_material(const matLightDef& mat, rgbf& occ, rgbf& emm)
{
    if (mat.isEmiting)
    {
        emm = mat.emitColor;
    }
    if (mat.isTransparent)
    {
        occ = mat.transparency;
    }
}
void apply_material(const matLightDef& mat, rgbf& occ, rgbf& emm,float area)
{
    if (mat.isEmiting)
    {
        emm += mat.emitColor*area;
    }
    if (mat.isTransparent)
    {
        occ *= mat.transparency.pow(area);
    }
}
/*
rgbf getStandartColor(int colorId)
{
    return rgbf(df::global::enabler->ccolor[colorId][0] / 255.0f,
        df::global::enabler->ccolor[colorId][1] / 255.0f,
        df::global::enabler->ccolor[colorId][2] / 255.0f);
}
int getPlantNumber(const std::string& id)
{
    std::vector<df::plant_raw*>& vec = df::plant_raw::get_vector();
    for (int i = 0; i<vec.size(); i++)
    {
        if (vec[i]->id == id)
            return i;
    }
    return -1;
}
void addPlant(const std::string& id, std::map<int, lightSource>& map, const lightSource& v)
{
    int nId = getPlantNumber(id);
    if (nId>0)
    {
        map[nId] = v;
    }
}
*/
void calculate_occlusion_and_emitters(const light_config & cfg, light_buffers & buffers, int offx, int offy, int offz, int x, int y, int z, int w, int h, int d)
{
    MapExtras::MapCache cache;

    int map_block_x_start = x / 16;
    int map_block_y_start = y / 16;
    int map_block_x_end = std::min((x + w) / 16, df::global::world->map.x_count_block);
    int map_block_y_end = std::min((y + h) / 16, df::global::world->map.y_count_block);
    for (int current_z = z; current_z < z + d; current_z++)
    {
        for (int blockX = map_block_x_start; blockX <= map_block_x_end; blockX++)
            for (int blockY = map_block_y_start; blockY <= map_block_y_end; blockY++)
            {
                MapExtras::Block* b = cache.BlockAt(DFCoord(blockX, blockY, current_z));
                MapExtras::Block* bDown = cache.BlockAt(DFCoord(blockX, blockY, current_z - 1));
                if (!b)
                    continue; //empty blocks fixed by sun propagation

                for (int block_x = 0; block_x < 16; block_x++)
                    for (int block_y = 0; block_y < 16; block_y++)
                    {
                        int tx = blockX * 16 + block_x;
                        int ty = blockY * 16 + block_y;

                        if (!buffers.coord_inside(tx + offx, ty + offy, current_z + offz))
                            continue;

                        df::coord2d gpos;
                        gpos.x = tx;
                        gpos.y = ty;

                        rgbf& cur_occlusion = buffers.get_occ(tx + offx, ty + offy, current_z + offz);
                        rgbf& cur_emitter = buffers.get_emt(tx + offx, ty + offy, current_z + offz);
                        cur_occlusion = cfg.matAmbience.transparency;


                        df::tiletype type = b->tiletypeAt(gpos);
                        df::tile_designation d = b->DesignationAt(gpos);
                        if (d.bits.hidden)
                        {
                            cur_occlusion = rgbf(0, 0, 0);
                            continue; // do not process hidden stuff, TODO other hidden stuff
                        }
                        //df::tile_occupancy o = b->OccupancyAt(gpos);
                        df::tiletype_shape shape = ENUM_ATTR(tiletype, shape, type);
                        bool is_wall = !ENUM_ATTR(tiletype_shape, passable_high, shape);
                        bool is_floor = !ENUM_ATTR(tiletype_shape, passable_low, shape);
                        df::tiletype_shape_basic basic_shape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
                        df::tiletype_material tileMat = ENUM_ATTR(tiletype, material, type);

                        DFHack::t_matpair mat = b->staticMaterialAt(gpos);

                        const matLightDef* lightDef = cfg.getMaterialDef(mat.mat_type, mat.mat_index);
                        if (!lightDef || !lightDef->isTransparent)
                            lightDef = &cfg.matWall;
                        if (shape == df::tiletype_shape::BROOK_BED)
                        {
                            cur_occlusion = rgbf(0, 0, 0);
                        }
                        else if (is_wall)
                        {
                            if (tileMat == df::tiletype_material::FROZEN_LIQUID)
                                apply_material(cfg.matIce, cur_occlusion, cur_emitter);
                            else
                                apply_material(*lightDef, cur_occlusion, cur_emitter);
                        }
                        else if (!d.bits.liquid_type && d.bits.flow_size > 0)
                        {
                            apply_material(cfg.matWater, cur_occlusion, cur_emitter, (float)d.bits.flow_size / 7.0f);
                        }
                        if (d.bits.liquid_type && d.bits.flow_size > 0)
                        {
                            apply_material(cfg.matLava, cur_occlusion, cur_emitter, (float)d.bits.flow_size / 7.0f);
                        }
                        else if (!is_floor)
                        {
                            if (bDown)
                            {
                                df::tile_designation d2 = bDown->DesignationAt(gpos);
                                if (d2.bits.liquid_type && d2.bits.flow_size > 0)
                                {
                                    apply_material(cfg.matLava, cur_occlusion, cur_emitter);
                                }
                            }
                        }
                    }

                df::map_block* block = b->getRaw();
                if (!block)
                    continue;

                //flows
                for (int i = 0; i < block->flows.size(); i++)
                {
                    df::flow_info* f = block->flows[i];
                    //TODO: add smoke, husking stuff
                    if (f && f->density > 0 && (f->type == df::flow_type::Dragonfire || f->type == df::flow_type::Fire))
                    {
                        int tx = f->pos.x + offx;
                        int ty = f->pos.y + offy;
                        int tz = current_z + offz;
                        if (buffers.coord_inside(tx, ty, tz))
                        {
                            rgbf fireColor;
                            if (f->density > 60)
                            {
                                fireColor = rgbf(0.98f, 0.91f, 0.30f);
                            }
                            else if (f->density > 30)
                            {
                                fireColor = rgbf(0.93f, 0.16f, 0.16f);
                            }
                            else
                            {
                                fireColor = rgbf(0.64f, 0.0f, 0.0f);
                            }
                            rgbf& cur_emitter = buffers.get_emt(tx + offx, ty + offy, current_z + offz);
                            cur_emitter += fireColor*(f->density / 5);
                        }
                    }
                }

                //blood and other goo
                for (int i = 0; i < block->block_events.size(); i++)
                {
                    df::block_square_event* ev = block->block_events[i];
                    df::block_square_event_type ev_type = ev->getType();
                    if (ev_type == df::block_square_event_type::material_spatter)
                    {
                        df::block_square_event_material_spatterst* spatter = static_cast<df::block_square_event_material_spatterst*>(ev);
                        const matLightDef* m = cfg.getMaterialDef(spatter->mat_type, spatter->mat_index);
                        if (!m)
                            continue;
                        if (!m->isEmiting)
                            continue;
                        for (int bx = 0; bx < 16; bx++)
                            for (int by = 0; by < 16; by++)
                            {
                                int16_t amount = spatter->amount[bx][by];
                                if (amount <= 0)
                                    continue;
                                int tx = blockX * 16 + bx + offx;
                                int ty = blockY * 16 + by + offy;
                                int tz = current_z + offz;
                                if (buffers.coord_inside(tx, ty, tz))
                                {
                                    buffers.get_emt(tx, ty, tz) += m->emitColor*((float)amount / 100);
                                }
                            }
                    }
                }
            }
    }
    if (df::global::cursor->x>-30000)
    {
        int tx = df::global::cursor->x + offx;
        int ty = df::global::cursor->y + offy;
        int tz = df::global::cursor->z + offz;
        if (buffers.coord_inside(tx, ty, tz))
        {
            rgbf& cur_occlusion = buffers.get_occ(tx,ty,tz);
            rgbf& cur_emitter = buffers.get_emt(tx, ty, tz);
            apply_material(cfg.matCursor, cur_occlusion, cur_emitter);
        }
    }
    /*
    //citizen only emit light, if defined
    //or other creatures
    if (matCitizen.isEmiting || creatureDefs.size()>0)
        for (int i = 0; i<df::global::world->units.active.size(); ++i)
        {
            df::unit *u = df::global::world->units.active[i];
            coord2d pos = worldToViewportCoord(coord2d(u->pos.x, u->pos.y), vp, window2d);
            if (u->pos.z == window_z && isInRect(pos, vp))
            {
                if (DFHack::Units::isCitizen(u) && !u->counters.unconscious)
                    addLight(getIndex(pos.x, pos.y), matCitizen.makeSource());
                creatureLightDef *def = getCreatureDef(u);
                if (def && !u->flags1.bits.dead)
                {
                    addLight(getIndex(pos.x, pos.y), def->light.makeSource());
                }
            }
        }
    //items
    if (itemDefs.size()>0)
    {
        std::vector<df::item*>& vec = df::global::world->items.other[items_other_id::IN_PLAY];
        for (size_t i = 0; i<vec.size(); i++)
        {
            df::item* curItem = vec[i];
            df::coord itemPos = DFHack::Items::getPosition(curItem);
            coord2d pos = worldToViewportCoord(itemPos, vp, window2d);
            itemLightDef* mat = 0;
            if (itemPos.z == window_z && isInRect(pos, vp) && (mat = getItemDef(curItem)))
            {
                if (((mat->equiped || mat->haul || mat->inBuilding || mat->inContainer) && curItem->flags.bits.in_inventory) || //TODO split this up
                    (mat->onGround && curItem->flags.bits.on_ground))
                {
                    if (mat->light.isEmiting)
                        addLight(getIndex(pos.x, pos.y), mat->light.makeSource());
                    if (!mat->light.isTransparent)
                        addOclusion(getIndex(pos.x, pos.y), mat->light.transparency, 1);
                }
            }
        }
    }

    //buildings
    for (size_t i = 0; i < df::global::world->buildings.all.size(); i++)
    {
        df::building *bld = df::global::world->buildings.all[i];

        if (window_z != bld->z)
            continue;
        if (bld->getBuildStage()<bld->getMaxBuildStage()) //only work if fully built
            continue;

        df::coord2d p1(bld->x1, bld->y1);
        df::coord2d p2(bld->x2, bld->y2);
        p1 = worldToViewportCoord(p1, vp, window2d);
        p2 = worldToViewportCoord(p2, vp, window2d);
        if (isInRect(p1, vp) || isInRect(p2, vp))
        {

            int tile;
            if (isInRect(p1, vp))
                tile = getIndex(p1.x, p1.y); //TODO multitile buildings. How they would work?
            else
                tile = getIndex(p2.x, p2.y);
            df::building_type type = bld->getType();
            buildingLightDef* def = getBuildingDef(bld);
            if (!def)
                continue;
            if (type == df::enums::building_type::Door)
            {
                df::building_doorst* door = static_cast<df::building_doorst*>(bld);
                if (!door->door_flags.bits.closed)
                    continue;
            }
            else if (type == df::enums::building_type::Floodgate)
            {
                df::building_floodgatest* gate = static_cast<df::building_floodgatest*>(bld);
                if (!gate->gate_flags.bits.closed)
                    continue;
            }


            if (def->useMaterial)
            {
                matLightDef* mat = getMaterialDef(bld->mat_type, bld->mat_index);
                if (!mat)mat = &matWall;
                if (!def->poweredOnly || !bld->isUnpowered()) //not powered. Add occlusion only.
                {
                    if (def->light.isEmiting)
                    {
                        addLight(tile, def->light.makeSource(def->size));
                    }
                    else if (mat->isEmiting)
                    {
                        addLight(tile, mat->makeSource(def->size));
                    }
                }
                if (def->light.isTransparent)
                {
                    addOclusion(tile, def->light.transparency, def->size);
                }
                else
                {
                    addOclusion(tile, mat->transparency, def->size);
                }
            }
            else
            {
                if (!def->poweredOnly || !bld->isUnpowered())//not powered. Add occlusion only.
                    addOclusion(tile, def->light.transparency, def->size);
                else
                    applyMaterial(tile, def->light, def->size, def->thickness);
            }
        }
    }
     }
     */
}
rgbf propogateSun(const light_config & cfg, MapExtras::Block* b, int x, int y, const rgbf& in, bool lastLevel)
{
    //TODO unify under addLight/addOclusion
    const rgbf matStairCase(0.9f, 0.9f, 0.9f);
    rgbf ret = in;
    coord2d innerCoord(x, y);
    df::tiletype type = b->staticTiletypeAt(innerCoord);
    df::tile_designation d = b->DesignationAt(innerCoord);
    //df::tile_occupancy o = b->OccupancyAt(innerCoord);
    df::tiletype_shape shape = ENUM_ATTR(tiletype, shape, type);
    df::tiletype_shape_basic basic_shape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
    DFHack::t_matpair mat = b->staticMaterialAt(innerCoord);
    df::tiletype_material tileMat = ENUM_ATTR(tiletype, material, type);

    const matLightDef* lightDef;
    if (tileMat == df::tiletype_material::FROZEN_LIQUID)
    {
        df::tiletype typeIce = b->tiletypeAt(innerCoord);
        df::tiletype_shape shapeIce = ENUM_ATTR(tiletype, shape, typeIce);
        df::tiletype_shape_basic basicShapeIce = ENUM_ATTR(tiletype_shape, basic_shape, shapeIce);
        if (basicShapeIce == df::tiletype_shape_basic::Wall)
            ret *= cfg.matIce.transparency;
        else if (basicShapeIce == df::tiletype_shape_basic::Floor || basicShapeIce == df::tiletype_shape_basic::Ramp || shapeIce == df::tiletype_shape::STAIR_UP)
            if (!lastLevel)
                ret *= cfg.matIce.transparency.pow(1.0f / 7.0f);
    }

    lightDef = cfg.getMaterialDef(mat.mat_type, mat.mat_index);

    if (!lightDef || !lightDef->isTransparent)
        lightDef = &cfg.matWall;
    if (basic_shape == df::tiletype_shape_basic::Wall)
    {
        ret *= lightDef->transparency;
    }
    else if (basic_shape == df::tiletype_shape_basic::Floor || basic_shape == df::tiletype_shape_basic::Ramp || shape == df::tiletype_shape::STAIR_UP)
    {

        if (!lastLevel)
            ret *= lightDef->transparency.pow(1.0f / 7.0f);
    }
    else if (shape == df::tiletype_shape::STAIR_DOWN || shape == df::tiletype_shape::STAIR_UPDOWN)
    {
        ret *= matStairCase;
    }
    if (d.bits.liquid_type == df::enums::tile_liquid::Water && d.bits.flow_size > 0)
    {
        ret *= cfg.matWater.transparency.pow((float)d.bits.flow_size / 7.0f);
    }
    else if (d.bits.liquid_type == df::enums::tile_liquid::Magma && d.bits.flow_size > 0)
    {
        ret *= cfg.matLava.transparency.pow((float)d.bits.flow_size / 7.0f);
    }
    return ret;
}
void calculate_sun(const light_config& cfg, light_buffers& buffers, float time, float angle_x, float angle_y, int offx, int offy, int offz,
    int x, int y, int z, int w, int h, int d)
{
    //TODO: d!=1 not done
    //angles not used!
    MapExtras::MapCache cache;
    float daycol;

    if (time<0)
    {
        int length = 1200 / cfg.daySpeed;
        daycol = (*df::global::cur_year_tick % length) / (float)length;
    }
    else
        daycol = fmod(time, 24.0f) / 24.0f; //1->12h 0->24h
    
    auto sky=cfg.get_sky_color(daycol);

    int map_block_x_start = x / 16;
    int map_block_y_start = y / 16;
    int map_block_x_end = std::min((x + w) / 16, df::global::world->map.x_count_block);
    int map_block_y_end = std::min((y + h) / 16, df::global::world->map.y_count_block);
    
    for (int blockX = map_block_x_start; blockX <= map_block_x_end; blockX++)
        for (int blockY = map_block_y_start; blockY <= map_block_y_end; blockY++)
        {
            rgbf cellArray[16][16];
            for (int block_x = 0; block_x < 16; block_x++)
                for (int block_y = 0; block_y < 16; block_y++)
                    cellArray[block_x][block_y] = sky;

            int emptyCell = 0;
            for (int current_z = z; current_z< df::global::world->map.z_count && emptyCell<256; current_z++)
            {
                MapExtras::Block* b = cache.BlockAt(DFCoord(blockX, blockY, current_z));
                if (!b)
                    continue;
                emptyCell = 0;
                for (int block_x = 0; block_x < 16; block_x++)
                    for (int block_y = 0; block_y < 16; block_y++)
                    {
                        rgbf& curCell = cellArray[block_x][block_y];
                        curCell = propogateSun(cfg,b, block_x, block_y, curCell, current_z == z);
                        if (curCell.dot(curCell)<0.003f)
                            emptyCell++;
                    }
            }
            if (emptyCell == 256)
                continue;
            for (int block_x = 0; block_x < 16; block_x++)
                for (int block_y = 0; block_y < 16; block_y++)
                {
                    rgbf& curCell = cellArray[block_x][block_y];
                    if(curCell.dot(curCell)>0.003f)
                    {
                        int tx = blockX * 16 + block_x + offx;
                        int ty = blockY * 16 + block_y + offy;
                        int tz = z + offz;
                    
                        if (buffers.coord_inside(tx, ty, tz))
                        {
                            buffers.get_emt(tx, ty, tz) = curCell;
                        }
                    }
                }
        }
}
