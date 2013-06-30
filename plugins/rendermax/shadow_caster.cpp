
#include "shadow_caster.hpp"
#include <math.h>

void shadow_octant::fill_arrays()
{
    index_array.resize(width);
    triangle_array.resize((width*(width+1))/2, light_cell(this));
    for(int i=0; i < index_array.size(); i++)
    {
        index_array[i] = (i*(i+1))/2;
        for(int j = 0; j <= i; j++)
        {
            triangle_array[index_array[i]+j].set_coords(df::coord2d(i,j));
        }
    }
    for (int i = 0; i < triangle_array.size(); i++)
    {
        triangle_array[i].fill_parents();
    }
}
int shadow_octant::get_index(int x, int y)
{
    if(x < width && y<=x)
        return index_array[x]+y;
    else
        return -1;
}

df::coord2d shadow_octant::get_coords(int index)
{
    return triangle_array[index].get_coords();
}

df::coord2d shadow_octant::translate_octant(df::coord2d input)
{
    switch (current_octant)
    {
    default: return input;
    case 1: return df::coord2d(input.y, input.x);
    case 2: return df::coord2d(-input.y, input.x);
    case 3: return df::coord2d(-input.x, input.y);
    case 4: return df::coord2d(-input.x, -input.y);
    case 5: return df::coord2d(-input.y, -input.x);
    case 6: return df::coord2d(input.y, -input.x);
    case 7: return df::coord2d(input.x, -input.y);
    }
}

df::coord2d shadow_octant::local_to_world(df::coord2d input)
{
    return translate_octant(input) + origin;
}

rgbf shadow_octant::get_opacity(df::coord2d coord)
{
    return owner->ocupancy[owner->getIndex(local_to_world(coord))];
}
lightSource shadow_octant::get_light(df::coord2d coord)
{
    return owner->get_light(coord);
}
void shadow_octant::blend_result(df::coord2d coord, rgbf input)
{
    int index = owner->getIndex(local_to_world(coord));
    rgbf oldcol =  (*destination)[index];
    oldcol = blend(oldcol, input);
    (*destination)[index] = oldcol;
}

void shadow_octant::reset()
{
    for( int i = 0; i < triangle_array.size(); i++)
    {
        triangle_array[i].reset();
    }
}

void shadow_octant::do_light(df::coord2d coord)
{
    this->origin = coord;
    rect2d vp = owner->mapPort;
    if(isInViewport(origin,vp))
    {
        lightSource cur_light = get_light(origin);
        if(cur_light.radius)
        {
            for(int i = 0; i < 8; i++)
            {
                reset();
                current_octant = i;
                if(fast)
                {
                    int open = 0;
                    triangle_array[get_index(0,0)].do_light(cur_light);
                    if(width > 1)
                    {
                        open += triangle_array[get_index(1,1)].do_light(cur_light);
                        open += triangle_array[get_index(1,0)].do_light(cur_light);
                    }
                    if((width > 2) && open)
                    {
                        for(int i = 0; i < width; i++)
                        {
                            triangle_array[get_index(width-1,i)].do_light(cur_light);
                        }
                    }
                }
                else
                {
                    for( int j = 0; j < triangle_array.size(); j++)
                    {
                        triangle_array[j].do_light(cur_light);
                    }
                }
            }
        }
    }
}

shadow_octant::shadow_octant(int radius, std::vector<rgbf> * destination, lightingEngineViewscreen * caller)
{
    owner = caller;
    width = radius;
    this->destination = destination;
    fill_arrays();
    fast=false;
}




void light_cell::set_coords(df::coord2d input)
{
    coords = input;
    distance_square = std::powf((float)input.x, 2) + std::powf((float)input.y, 2);
    distance = std::sqrtf(distance_square);
}

void light_cell::fill_parents()
{
    parents.resize(coords.x);
    for(int i = 0; i < coords.x; i++)
    {
        float y = coords.x ? (float)(coords.y*i)/(float)coords.x : 0;
        parents[i].first = floor(y);
        parents[i].fraction = y-parents[i].first;
        if(parents[i].fraction < 0.00001) //if there's not enough difference between them, don't bother blending
        {
            parents[i].single = true;
            parents[i].fraction = 0.0f;
        }
        else parents[i].single = false;
        owner->get_cell(i,parents[i].first)->add_child(this);
        if(!parents[i].single)
            owner->get_cell(i,parents[i].first+1)->add_child(this);
    }
}

void light_cell::kill(light_cell* killer)
{
    if(killer != this)
    {
        if(parents[killer->coords.x].first == killer->coords.y)
            parents[killer->coords.x].blocked_first = true;
        else if(!parents[killer->coords.x].single && (parents[killer->coords.x].first+1 == killer->coords.y))
            parents[killer->coords.x].blocked_second = true;
        if(parents[killer->coords.x].blocked_first && parents[killer->coords.x].blocked_second)
        {
            dead = true;
            for(int i = 0; i < children.size(); i++)
                children[i]->kill(this);
        }
    }
    else
        dead = true;
}

void light_cell::reset()
{
    for(int i = 0; i < parents.size(); i++)
    {
        parents[i].blocked_first = false;
        parents[i].blocked_second = false;
    }
    dead = false;
}

bool light_cell::do_light(lightSource light)
{
    if(dead)
        return false;
    if((distance_square >= std::pow((float)light.radius,2)) && !owner->fast)
    {
        kill(this);
        return false;
    }
    if(!owner->is_in_viewport(coords) && !owner->fast)
    {
        kill(this);
        return false;
    }
    rgbf final_color=light.power;
    for(int i = 0; i < parents.size(); i++)
    {
        if(parents[i].fraction > 0.5)
        {
            if(!owner->is_in_viewport(df::coord2d(i, parents[i].first+1)) && !owner->fast)
            {
                kill(this);
                return false;
            }
        }else
        {
            if(!owner->is_in_viewport(df::coord2d(i, parents[i].first)) && !owner->fast)
            {
                kill(this);
                return false;
            }
        }
        if(i > 0)
        {
            if(light.power <= owner->get_light_local(df::coord2d(i, parents[i].first)).power) //don't bother checking both when one will suffice.
            {
                kill(this);
        return false;
            }
            if(!parents[i].single && (light.power <= owner->get_light_local(df::coord2d(i, parents[i].first+1)).power)) //but check em both anyway
            {
                kill(this);
        return false;
            }
        }
        if(owner->fast)
        {
            rgbf adjusted_color;
            if(coords.y > 0) //round that fucker
                adjusted_color=final_color.pow(distance/(float)coords.x);
            else
                adjusted_color=final_color;
            if(parents[i].fraction > 0.5)
                owner->blend_result(df::coord2d(i, parents[i].first+1), adjusted_color);
            else
                owner->blend_result(df::coord2d(i, parents[i].first), adjusted_color);

        }
        if(parents[i].single)
            final_color*=owner->get_opacity(df::coord2d(i, parents[i].first));
        else
        {
            final_color*=(
                (owner->get_opacity(df::coord2d(i, parents[i].first))*(1.0f-parents[i].fraction)) + 
                (owner->get_opacity(df::coord2d(i, parents[i].first+1))*(parents[i].fraction)));
        }
        if(final_color.r <= owner->owner->levelDim && final_color.g <= owner->owner->levelDim && final_color.b <= owner->owner->levelDim)
        {
            kill(this);
        return false;
        }
    }
    //final_color*=owner->get_opacity(coords);
    //if(final_color.r <= owner->owner->levelDim && final_color.g <= owner->owner->levelDim && final_color.b <= owner->owner->levelDim)
    //{
    //    kill(this);
    //    return;
    //}
    if(coords.y > 0) //round that fucker
    {
        final_color=final_color.pow(distance/(float)coords.x);
    }
    owner->blend_result(coords, final_color);
    return true;
}
