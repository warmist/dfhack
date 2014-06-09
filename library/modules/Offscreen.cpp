#include "modules/Offscreen.h"

#include <vector>

#include "Core.h"
#include "Console.h"
#include "Export.h"

#include "DataDefs.h"
#include "Types.h"
#include "TileTypes.h"

#include "Error.h"

#include "modules/MapCache.h"
#include "modules/Materials.h"
#include "modules/Units.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/building_drawbuffer.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "df/plant.h"
#include "df/profession.h"
#include "df/flow_info.h"
#include "df/matter_state.h"
#include "df/descriptor_color.h"
#include "df/matter_state.h"

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

bool Offscreen::isInRect( const df::coord2d& pos,const rect2d& rect )
{
    if(pos.x>=rect.first.x && pos.y>=rect.first.y && pos.x<rect.second.x && pos.y<rect.second.y)
        return true;
    return false;
}

Offscreen::Offscreen()
{

}

Offscreen::~Offscreen()
{

}

bool Offscreen::Finish()
{
    return true;
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

void Offscreen::drawBuffer( rect2d window,int z,std::vector<screenTile>& buffer )
{
    if(!df::global::world)
        return;

    //TODO static array of images for each tiletype
    MapExtras::MapCache cache;
    int w=window.second.x-window.first.x+1;
    int h=window.second.y-window.first.y+1;
    rect2d localWindow=mkrect_wh(0,0,w,h);
    if(buffer.size()!=w*h)
        buffer.resize(w*h);
    //basic tiletype stuff here
    for(int x=window.first.x;x<=window.second.x;x++) //todo, make it by block, minimal improvement over cache prob though
        for(int y=window.first.y;y<=window.second.y;y++)
        {
            DFCoord coord(x,y,z);
            df::tiletype tt=cache.tiletypeAt(coord);
            df::tiletype_shape shape = ENUM_ATTR(tiletype,shape,tt);
            df::tiletype_shape_basic basic_shape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
            t_matpair mat=cache.staticMaterialAt(coord);
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
                if(!inliquid)
                    colorTile(tileMat,cache,coord,curTile,true);
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
                std::vector<df::block_square_event*>& events=b->getRaw()->block_events;
                for(size_t i=0;i<events.size();i++)//maybe aggregate all the events to one array and move to a function.
                {
                    df::block_square_event* e=events[i];
                    switch(e->getType())
                    {
                    case df::block_square_event_type::grass:
                        {
                            df::block_square_event_grassst* grass=static_cast<df::block_square_event_grassst*>(e);
                            MaterialInfo mat(419, grass->plant_index);
                            if(mat.isPlant())
                            {
                                df::plant_raw* p=mat.plant;
                                for(int x=0;x<16;x++)
                                for(int y=0;y<16;y++)
                                {
                                    int wx=x+bx*16-window.first.x;
                                    int wy=y+by*16-window.first.y;
                                    if(isInRect(df::coord2d(wx,wy),localWindow) && grass->amount[x][y]>0)
                                    {
                                        screenTile& curTile=buffer[wx*h+wy];
                                        /*
                                        df::tiletype tt=b->tiletypeAt(df::coord2d(x,y));
                                        df::tiletype_special sp=ENUM_ATTR(tiletype,special,tt);
                                        df::tiletype_special::DEAD;
                                        df::tiletype_special::WET;
                                        df::tiletype_special::NORMAL;
                                        +variants
                                        */

                                        curTile.tile=p->tiles.grass_tiles[0];
                                        curTile.fg=p->colors.grass_colors_0[0];
                                        curTile.bg=p->colors.grass_colors_1[0];
                                        curTile.bold=p->colors.grass_colors_2[0];
                                    }
                                    
                                    
                                }
                                
                                
                                
                                //TODO alt-tiles
                            }
                            
                            break;
                        }
                    case df::block_square_event_type::material_spatter:
                        {
                            //liquid:
                            //0 nothing 
                            //1->49 color 
                            //50->99 wave
                            //100->255 two waves
                            //color only, if small
                            //draw waves, if pool
                            df::block_square_event_material_spatterst* spatter=static_cast<df::block_square_event_material_spatterst*>(e);
                            MaterialInfo mat(spatter);
                            if(mat.material)
                            {

                            for(int x=0;x<16;x++)
                                for(int y=0;y<16;y++)
                                {
                                    int wx=x+bx*16-window.first.x;
                                    int wy=y+by*16-window.first.y;
                                    uint8_t amount=spatter->amount[x][y];
                                    if(isInRect(df::coord2d(wx,wy),localWindow) && amount>0)
                                    {
                                        screenTile& curTile=buffer[wx*h+wy];
                                        
                                        curTile.fg=mat.material->tile_color[0];
                                        curTile.bold=mat.material->tile_color[2];
                                        if(spatter->mat_state==df::matter_state::Liquid && amount>49)
                                        {
                                            if(amount>99)
                                                curTile.tile=247;
                                            else
                                                curTile.tile=126;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    default:;
                    }
                }
                std::vector<df::flow_info*>& flows=b->getRaw()->flows;
                for(size_t i=0;i<flows.size();i++)
                {
                    df::flow_info* f=flows[i];
                    int wx=f->pos.x-window.first.x;
                    int wy=f->pos.y-window.first.y;
                    if(f->density>0 && isInRect(df::coord2d(wx,wy),localWindow))
                    {
                        screenTile& curTile=buffer[wx*h+wy];
                        drawFlow(f,curTile);
                    }
                }
            }
            //in df items blink between stuff, but i don't have time for that
            //also move up, before flows
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
                    drawUnit(u,curTile);
                }
            }
            
}
bool isMilitary(df::unit *u)
{
    return u->profession>=df::profession::HAMMERMAN && u->profession<=df::profession::RECRUIT;
}
void Offscreen::drawUnit( df::unit* u,screenTile& trg )
{
    df::creature_raw* cr=df::creature_raw::find(u->race);
    bool soldier=isMilitary(u);
    
    if(cr)
    {
        //u->flags2.bits.swimming -> special swiming stuff?
        //u->flags3.bits.ghostly -> ghostly stuff
        //grapling
        //onground
        //sleeping ->blink?
        if(soldier)
            trg.tile=cr->creature_soldier_tile;
        else
            trg.tile=cr->creature_tile;
        
        trg.fg=Units::getProfessionColor(u);//do this only for those who have profesions?
        trg.bg=cr->color[1];
        trg.bold=cr->color[2];
        //TODO glowtiles ,BLINKING
        
        if(u->caste<cr->caste.size())
        {
            df::caste_raw* casteRaw=cr->caste[u->caste];
            //trg.tile=casteRaw->caste_soldier_tile;
            if(!casteRaw->flags.bits[df::caste_raw_flags::CASTE_TILE])
            {
                if(soldier)
                    trg.tile=casteRaw->caste_soldier_tile;
                else
                    trg.tile=casteRaw->caste_tile;
            }
            if(!casteRaw->flags.bits[df::caste_raw_flags::CASTE_COLOR])
            {
                trg.fg=Units::getProfessionColor(u);
                trg.bg=casteRaw->caste_color[1];
                trg.bold=casteRaw->caste_color[2];
            }
        }
        
    }
}

void Offscreen::drawPlant( df::plant* p,screenTile& trg )
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
        //sapling ?
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

void Offscreen::drawItem( df::item* it,screenTile& trg )
{
    trg.tile='?';
    trg.fg=COLOR_RED;
    trg.bold=true;
}

void DFHack::Offscreen::drawFlow( df::flow_info* flow,screenTile& trg )
{
    const int tilesFlow[]={176,177,178,219}; //configure?
    //dragonfire and fire turns yellow at 50%
    //webs, sea foam, ocean waves are special
    //else different tile each 25 till 100

    //TODO sea foam, waves, materials
    if(flow->density==0)
        return;
    int d=flow->density/25;
    if(d>3)
        d=3;
    int tile=tilesFlow[d];
    trg.tile=tile;
    switch(flow->type)
    {
    case df::flow_type::Miasma:
        trg.fg=5;//purple
        break;
    case df::flow_type::Dragonfire:
    case df::flow_type::Fire:
        {
            trg.fg=COLOR_RED;
            if(d>1)
                trg.fg=COLOR_YELLOW;
            if(d>0)
                trg.bold=true;
            return;
        }
    case df::flow_type::MagmaMist:
         trg.fg=COLOR_YELLOW;
         trg.bold=true;
         return;
    case df::flow_type::Smoke:
        trg.fg=0;
        trg.bold=true;
        return;
    case df::flow_type::Mist:
    case df::flow_type::Steam:
        trg.fg=COLOR_WHITE;
        trg.bold=true;
        return;
    case df::flow_type::MaterialGas:
    case df::flow_type::MaterialVapor:
        trg.fg=COLOR_RED;
        trg.bold=true;
        return;
    case df::flow_type::MaterialDust:
        trg.fg=0;
        trg.bold=true;
        return;
    case df::flow_type::Web:
        trg.fg=0;
        trg.bold=true;
        trg.tile=15;
        return;
    }
}
