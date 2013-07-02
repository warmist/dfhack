//makes planting trees easier

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "df/building_farmplotst.h"
#include "df/plant.h"
#include "df/tiletype.h"
#include "df/tiletype_shape.h"
#include "modules/Maps.h"

using df::building_farmplotst;
using df::plant;
using df::tiletype;
using df::tiletype_shape;
using namespace DFHack;

DFHACK_PLUGIN("grove");
plant* getNearest(int x,int y,int z,int radius) //TODO optimise to use mapcache or sth...
{
    int radSq=radius*radius;
    for(int i=-radius;i<+radius;i++)
    for(int j=-radius;j<+radius;j++)
    {
        if(i*i+j*j>radSq)
            continue;
        int tx=x+i;
        int ty=y+j;
        tiletype* tt=Maps::getTileType(tx,ty,z);
        if(!tt)
            continue;
        tiletype_shape shape=ENUM_ATTR(tiletype,tiletype_shape,*tt);
        map_block* b=Maps::getTileBlock(tx,ty,z);
        if(!b)
            continue;
        
    }
}
struct farm_hook:public building_farmplotst
{
    typedef building_farmplotst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(
        void,updateAction,()
    {
        if(max_fertilization==current_fertilization && //maximum fertilization
            x2-x1==0 && //and is 1x1
            y2-y1==0 &&
            getBuildStage()==getMaxBuildStage() //and is fully built
            )
        {

        }
        INTERPOSE_NEXT(updateAction)();
    }
};
command_result df_grove (color_ostream &out, vector <string> & parameters)
{

}
DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "grove", "Allow planting trees by using fully fertilized 1x1 plot near other trees",
        df_grove, false,
        "  enable \n"
        "  disable \n"
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
