#pragma once

#include <vector>

#include "renderer_light.hpp"
#include "Types.h"

////Octants!
//\2|1/
//3\|/0
//--+--
//4/|\7
///5|6\

class shadow_octant;
class light_cell_pair;
class light_cell
{
private:
    bool dead;
    std::vector<light_cell_pair> parents; //list of cells that light must pass through to get here
    std::vector<light_cell *> children; //elements that are in complete darkness if this is.
    float distance;
    float distance_square;
    shadow_octant * owner;
    df::coord2d coords;
public:
    light_cell(shadow_octant * owner) 
    {
        this->owner = owner;
        dead = false;
    }
    void fill_parents();
    void kill(light_cell * killer);
    void add_child(light_cell * child){children.push_back(child);}
    void reset();
    bool do_light(lightSource light);
    void set_coords(df::coord2d);
    df::coord2d get_coords(){return coords;}
};

class light_cell_pair
{
public:
    int first;
    float fraction;
    bool single; //technically can use fraction, but this is less ambigouous
    bool blocked_first, blocked_second;
    light_cell_pair():blocked_first(false),blocked_second(false){}
};

class shadow_octant
{
private:
    df::coord2d origin;
    int width;
    std::vector<light_cell> triangle_array;
    std::vector<int> index_array; //shortcut to get the number of cells before an X value
    void fill_arrays();
    df::coord2d translate_octant(df::coord2d input);
    df::coord2d local_to_world(df::coord2d input);
    unsigned char current_octant;
    std::vector<rgbf> * destination;
    lightSource get_light(df::coord2d);
public:
    bool fast;
    void reset();
    lightingEngineViewscreen * owner;
    void do_light(df::coord2d coord);
    int get_index(int x, int y);
    int get_index(df::coord2d coords)
    {
        return get_index(coords.x, coords.y);
    }
    df::coord2d get_coords(int index);
    light_cell * get_cell(int index)
    {
        if(index >=0)
            return &(triangle_array[index]);
        return nullptr;
    }
    light_cell * get_cell(int x, int y)
    {
        return get_cell(get_index(x,y));
    }
    rgbf get_opacity(df::coord2d);
    lightSource get_light_local(df::coord2d input){return get_light(local_to_world(input));}
    void blend_result(df::coord2d, rgbf);
    shadow_octant(int radius, std::vector<rgbf> * destination, lightingEngineViewscreen * caller);
    bool is_in_viewport(df::coord2d input)
    {
        return isInViewport(local_to_world(input),owner->mapPort);
    }

};
