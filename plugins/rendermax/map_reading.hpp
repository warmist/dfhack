#pragma once
/*
this header provides functions to calculate initial light positions and obstructions for any part of map
*/

#include <unordered_map>

#include "Types.h"
#include "renderer_opengl.hpp"


// we are not using boost so let's cheat:
template <class T>
inline void hash_combine(std::size_t & seed, const T & v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
    template<typename S, typename T> struct hash<pair<S, T>>
    {
        inline size_t operator()(const pair<S, T> & v) const
        {
            size_t seed = 0;
            ::hash_combine(seed, v.first);
            ::hash_combine(seed, v.second);
            return seed;
        }
    };
    template<typename S, typename T, typename V> struct hash<tuple<S, T, V>>
    {
        inline size_t operator()(const tuple<S, T, V> & v) const
        {
            size_t seed = 0;
            ::hash_combine(seed, get<0>(v));
            ::hash_combine(seed, get<1>(v));
            ::hash_combine(seed, get<2>(v));
            return seed;
        }
    };
}
// now we can hash pairs and tuples
struct matLightDef
{
    bool isTransparent;
    rgbf transparency;
    bool isEmiting;
    bool flicker;
    rgbf emitColor;
    int radius;
    matLightDef() :isTransparent(false), isEmiting(false), transparency(0, 0, 0), emitColor(0, 0, 0), radius(0) {}
    matLightDef(rgbf transparency, rgbf emit, int rad) :isTransparent(true), isEmiting(true),
        transparency(transparency), emitColor(emit), radius(rad) {}
    matLightDef(rgbf emit, int rad) :isTransparent(false), isEmiting(true), emitColor(emit), radius(rad), transparency(0, 0, 0) {}
    matLightDef(rgbf transparency) :isTransparent(true), isEmiting(false), transparency(transparency) {}
};
struct buildingLightDef
{
    matLightDef light;
    bool poweredOnly;
    bool useMaterial;
    float thickness;
    float size;
    buildingLightDef() :poweredOnly(false), useMaterial(true), thickness(1.0f), size(1.0f) {}
};
struct itemLightDef
{
    matLightDef light;
    bool haul;
    bool equiped;
    bool onGround;
    bool inBuilding;
    bool inContainer;
    bool useMaterial;
    itemLightDef() :haul(true), equiped(true), onGround(true), inBuilding(false), inContainer(false), useMaterial(true) {}
};
struct creatureLightDef
{
    matLightDef light;

};

struct light_config //defines all materials and stuff
{
    void defaultSettings();//set up sane settings if setting file does not exist.

    rgbf get_sky_color(float v) const;
    std::vector<rgbf> dayColors; // a gradient of colors, first to 0, last to 24

    const matLightDef* getMaterialDef(int matType, int matIndex) const;
    const buildingLightDef* getBuildingDef(df::building* bld) const;
    const creatureLightDef* getCreatureDef(df::unit* u) const;
    const itemLightDef* getItemDef(df::item* it) const;

    static int parseMaterials(lua_State* L);
    static int parseSpecial(lua_State* L);
    static int parseBuildings(lua_State* L);
    static int parseItems(lua_State* L);
    static int parseCreatures(lua_State* L);
    //special stuff
    matLightDef matLava;
    matLightDef matIce;
    matLightDef matAmbience;
    matLightDef matCursor;
    matLightDef matWall;
    matLightDef matWater;
    matLightDef matCitizen;

    
    float levelDim;
    int num_bounces;
    float daySpeed;
    float dayHour;
    int adv_mode;//this the mode of adventure light calculation (0/1)
    int max_radius;
    float adapt_speed; //how much to adapt [0,1] 1 is instant, 0 is do not use this
    float adapt_brightness;
    //materials
    std::unordered_map<std::pair<int, int>, matLightDef> matDefs;
    //buildings
    std::unordered_map<std::tuple<int, int, int>, buildingLightDef> buildingDefs;
    //creatures
    std::unordered_map<std::pair<int, int>, creatureLightDef> creatureDefs;
    //items
    std::unordered_map<std::pair<int, int>, itemLightDef> itemDefs;

    void load_settings();//load up settings from lua
};

struct light_buffers
{
    int w, h, d;
    std::vector<rgbf> occlusion;
    std::vector<rgbf> emitters;
    std::vector<rgbf> light;
    void update_size()
    {
        occlusion.resize(w*h*d, rgbf(0, 0, 0));
        emitters.resize(w*h*d, rgbf(0, 0, 0));
        light.resize(w*h*d, rgbf(0, 0, 0));
    }
    rgbf& get_occ(int x, int y, int z) 
    {
        return occlusion[x*h + y + z*w*h];
    };

    const rgbf& get_occ(int x, int y, int z) const
    {
        return occlusion[x*h + y + z*w*h];
    }
    rgbf& get_emt(int x, int y, int z)
    {
        return emitters[x*h + y + z*w*h];
    };

    const rgbf& get_emt(int x, int y, int z) const
    {
        return emitters[x*h + y + z*w*h];
    }
    rgbf& get_lgt(int x, int y, int z)
    {
        return light[x*h + y + z*w*h];
    };

    const rgbf& get_lgt(int x, int y, int z) const
    {
        return light[x*h + y + z*w*h];
    }
    bool coord_inside(int x, int y, int z) const
    {
        if (x < 0 || y < 0 || z < 0 || x >= w || y >= h || z >= d)
            return false;
        return true;
    }
};

void calculate_occlusion_and_emitters(const light_config& cfg,light_buffers& buffers,int offx,int offy,int offz,
    int x,int y,int z,int w,int h,int d=1);
void calculate_sun(const light_config& cfg, light_buffers& buffers,float time,float angle_x,float angle_y, int offx, int offy, int offz,
    int x, int y, int z, int w, int h, int d = 1);
