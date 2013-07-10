/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2013 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once
namespace df
{
    struct unit;
    struct plant;
    struct item;
    struct coord2d;
}


#include "Core.h"
#include "Export.h"
#include "Module.h"
#include "Types.h"

namespace DFHack
{
    //a visual representation of stuff in df on screen
    struct screenTile
    {
        int tile;
        int8_t fg, bg;
        bool bold;
    };
    class DFHACK_EXPORT Offscreen:public Module
    {
        bool isInRect(const df::coord2d& pos,const rect2d& rect);
    public:
        Offscreen();
        ~Offscreen();
        bool Finish();

        void drawBuffer(rect2d window,int z,std::vector<screenTile>& buffer);   //draws a fully complete map from window defined by coordinates to a buffer

        void drawUnit(df::unit* u,screenTile& trg); 
        void drawPlant(df::plant* p,screenTile& trg); //not grass, only trees, bushes, saplings etc...
        void drawItem(df::item* it,screenTile& trg); //item that is drawn like it would be lying on the ground
    };
}