#include "renderer_light.hpp"

#include <functional>
#include <math.h>
#include <string>
#include <vector>

#include "tinythread.h"

#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_dungeonmodest.h"
#include "modules/Gui.h"
#include "modules/MapCache.h"
using df::global::gps;
using namespace DFHack;
using df::coord2d;
using namespace tthread;

const float RootTwo = 1.4142135623730950488016887242097f;


bool isInRect(const coord2d& pos,const rect2d& rect)
{
    if(pos.x>=rect.first.x && pos.y>=rect.first.y && pos.x<rect.second.x && pos.y<rect.second.y)
        return true;
    return false;
}

lightingEngineViewscreen::~lightingEngineViewscreen()
{
    
}

rect2d getMapViewport()
{
    const int AREA_MAP_WIDTH = 23;
    const int MENU_WIDTH = 30;
    
    auto view = Gui::getCurViewscreen();
    if(!view || !gps)
        return mkrect_wh(0, 0, 0, 0);
   
    bool is_dwarf_mode = df::viewscreen_dwarfmodest::_identity.is_instance(view);
    bool is_adv_mode = df::viewscreen_dungeonmodest::_identity.is_instance(view);
    if (is_adv_mode)
    {
        return mkrect_wh(0, 0, gps->dimx, gps->dimy);
    }
    if(!is_dwarf_mode)
        return mkrect_wh(0, 0, 0, 0);

    int w=gps->dimx;
    int h=gps->dimy;
    int view_height=h-2;
    int area_x2 = w-AREA_MAP_WIDTH-2;
    int menu_x2=w-MENU_WIDTH-2;
    int menu_x1=area_x2-MENU_WIDTH-1;
    int view_rb=w-1;

    int area_pos=*df::global::ui_area_map_width;
    int menu_pos=*df::global::ui_menu_width;
    if(area_pos<3)
    {
        view_rb=area_x2;
    }
    if (menu_pos<area_pos || df::global::ui->main.mode!=0)
    {
        if (menu_pos >= area_pos)
            menu_pos = area_pos-1;
        int menu_x = menu_x2;
        if(menu_pos < 2) menu_x = menu_x1;
        view_rb = menu_x;
    }
    return mkrect_wh(1,1,view_rb,view_height+1);
}
lightingEngineViewscreen::lightingEngineViewscreen(renderer_light* target):lightingEngine(target),doDebug(0)
{
    reinit();
    cfg.defaultSettings();
    int numTreads=tthread::thread::hardware_concurrency();

}

void lightingEngineViewscreen::reinit()
{
    if(!gps)
        return;
    buffers.w = gps->dimx;
    buffers.h = gps->dimy;
    buffers.d = 1;

    buffers.update_size();
    was_light = 1;
}

void plotCircle(int xm, int ym, int r,const std::function<void(int,int)>& setPixel)
{
    int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */
    do {
        setPixel(xm-x, ym+y); /*   I. Quadrant */
        setPixel(xm-y, ym-x); /*  II. Quadrant */
        setPixel(xm+x, ym-y); /* III. Quadrant */
        setPixel(xm+y, ym+x); /*  IV. Quadrant */
        r = err;
        if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
        if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
    } while (x < 0);
}
void plotSquare(int xm, int ym, int r,const std::function<void(int,int)>& setPixel)
{
    for(int x = 0; x <= r; x++)
    {
        setPixel(xm+r, ym+x); /*   I.1 Quadrant */
        setPixel(xm+x, ym+r); /*   I.2 Quadrant */
        setPixel(xm+r, ym-x); /*   II.1 Quadrant */
        setPixel(xm+x, ym-r); /*   II.2 Quadrant */
        setPixel(xm-r, ym-x); /*   III.1 Quadrant */
        setPixel(xm-x, ym-r); /*   III.2 Quadrant */
        setPixel(xm-r, ym+x); /*   IV.1 Quadrant */
        setPixel(xm-x, ym+r); /*   IV.2 Quadrant */
    }
}
void plotLine(int x0, int y0, int x1, int y1,rgbf power,const std::function<rgbf(rgbf,int,int,int,int)>& setPixel)
{
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2; /* error value e_xy */
    int rdx=0;
    int rdy=0;
    for(;;){  /* loop */
        if(rdx!=0 || rdy!=0) //dirty hack to skip occlusion on the first tile.
        {
            power=setPixel(power,rdx,rdy,x0,y0);
            if(power.dot(power)<0.00001f)
                return ;
        }
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        rdx=rdy=0;
        if (e2 >= dy) { err += dy; x0 += sx; rdx=sx;} /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; rdy=sy;} /* e_xy+e_y < 0 */
    }
    return ;
}
void plotLineDiffuse(int x0, int y0, int x1, int y1,rgbf power,int num_diffuse,const std::function<rgbf(rgbf,int,int,int,int)>& setPixel,bool skip_hack=false)
{

    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int dsq=dx*dx+dy*dy;
    int err = dx+dy, e2; /* error value e_xy */
    int rdx=0;
    int rdy=0;
    for(;;){  /* loop */
        if(rdx!=0 || rdy!=0 || skip_hack) //dirty hack to skip occlusion on the first tile.
        {
            power=setPixel(power,rdx,rdy,x0,y0);
            if(power.dot(power)<0.00001f)
                return ;
        }
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        rdx=rdy=0;
        if (e2 >= dy) { err += dy; x0 += sx; rdx=sx;} /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; rdy=sy;} /* e_xy+e_y < 0 */

        if(num_diffuse>0 && dsq/4<(x1-x0)*(x1-x0)+(y1-y0)*(y1-y0))//reached center?
        {
            const float betta=0.25;
            int nx=y1-y0; //right angle
            int ny=x1-x0;
            if((nx*nx+ny*ny)*betta*betta>2)
            {
                plotLineDiffuse(x0,y0,x0+nx*betta,y0+ny*betta,power,num_diffuse-1,setPixel,true);
                plotLineDiffuse(x0,y0,x0-nx*betta,y0-ny*betta,power,num_diffuse-1,setPixel,true);
            }
        }
    }
    return ;
}
void plotLineAA(int x0, int y0, int x1, int y1,rgbf power,const std::function<rgbf(rgbf,int,int,int,int)>& setPixelAA)
{
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx-dy, e2, x2;                       /* error value e_xy */
    int ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);
    int rdx=0;
    int rdy=0;
    int lrdx,lrdy;
    rgbf sumPower;
    for ( ; ; ){                                         /* pixel loop */
        float strsum=0;
        float str=1-abs(err-dx+dy)/(float)ed;
        strsum=str;
        sumPower=setPixelAA(power*str,rdx,rdy,x0,y0);
        e2 = err; x2 = x0;
        lrdx=rdx;
        lrdy=rdy;
        rdx=rdy=0;
        if (2*e2 >= -dx) {                                    /* x step */
            if (x0 == x1) break;

            if (e2+dy < ed)
                {
                    str=1-(e2+dy)/(float)ed;
                    sumPower+=setPixelAA(power*str,lrdx,lrdy,x0,y0+sy);
                    strsum+=str;
                }
            err -= dy; x0 += sx; rdx=sx;
        }
        if (2*e2 <= dy) {                                     /* y step */
            if (y0 == y1) break;

            if (dx-e2 < ed)
                {
                    str=1-(dx-e2)/(float)ed;
                    sumPower+=setPixelAA(power*str,lrdx,lrdy,x2+sx,y0);
                    strsum+=str;
                }
            err += dx; y0 += sy; rdy=sy;
        }
        if(strsum<0.001f)
            return;
        sumPower=sumPower/strsum;
        if(sumPower.dot(sumPower)<0.00001f)
            return;
        power=sumPower;
    }
}
void lightingEngineViewscreen::clear()
{
    auto& lmap = buffers.light;
    lmap.assign(lmap.size(),rgbf(1,1,1));
    tthread::lock_guard<tthread::fast_mutex> guard(myRenderer->dataMutex);
    if(lmap.size()==myRenderer->lightGrid.size())
    {
        std::swap(myRenderer->lightGrid,lmap);
        myRenderer->invalidate();
    }
}
void clamp_light(light_buffers& l, rect2d r)
{
    //TODO: offsets?
    for (int x = r.first.x; x<r.second.x; x++)
        for (int y = r.first.y; y<r.second.y; y++)
        {
            auto&c = l.get_lgt(x, y, 0);
            if (c.r > 1)c.r = 1;
            if (c.g > 1)c.g = 1;
            if (c.b > 1)c.b = 1;
        }
}
rgbf rgb2xyz(rgbf c) {
    rgbf tmp;
    tmp.r = (c.r > 0.04045) ? pow((c.r + 0.055) / 1.055, 2.4) : c.r / 12.92;
    tmp.g = (c.g > 0.04045) ? pow((c.g + 0.055) / 1.055, 2.4) : c.g / 12.92;
    tmp.b = (c.b > 0.04045) ? pow((c.b + 0.055) / 1.055, 2.4) : c.b / 12.92;
    const rgbf line1(0.4124, 0.3576, 0.1805);
    const rgbf line2(0.2126, 0.7152, 0.0722);
    const rgbf line3(0.0193, 0.1192, 0.9505);

    return rgbf(tmp.dot(line1), tmp.dot(line2), tmp.dot(line3), 0);
}

rgbf xyz2rgb(rgbf c) {
    const rgbf line1(3.2406, -1.5372, -0.4986);
    const rgbf line2(-0.9689, 1.8758, 0.0415);
    const rgbf line3(0.0557, -0.2040, 1.0570);
    rgbf v = rgbf(line1.dot(c), line2.dot(c), line3.dot(c));
    rgbf r;
    r.r = (v.r > 0.0031308) ? ((1.055 * pow(v.r, (1.0 / 2.4))) - 0.055) : 12.92 * v.r;
    r.g = (v.g > 0.0031308) ? ((1.055 * pow(v.g, (1.0 / 2.4))) - 0.055) : 12.92 * v.g;
    r.b = (v.b > 0.0031308) ? ((1.055 * pow(v.b, (1.0 / 2.4))) - 0.055) : 12.92 * v.b;
    return r;
}
void rgb_to_cie(light_buffers& l,rect2d r)
{
    //TODO: offsets?
    for(int x=r.first.x;x<r.second.x;x++)
        for(int y=r.first.y;y<r.second.y;y++)
    {
        auto&c = l.get_lgt(x, y, 0);
        c= rgb2xyz(c);
    }
}
void cie_to_rgb(light_buffers& l, rect2d r)
{
    //TODO: offsets?
    for (int x = r.first.x; x<r.second.x; x++)
        for (int y = r.first.y; y<r.second.y; y++)
        {
            auto&c = l.get_lgt(x, y, 0);
            c = xyz2rgb(c);
        }
}
void light_adapt(light_buffers& l, rect2d r, float adapt_lvl, float min_level, float brightness, float& was_light)
{
    if (adapt_lvl > 1)adapt_lvl = 1;
    float max_light = 0;
    for (int x = r.first.x; x<r.second.x; x++)
        for (int y = r.first.y; y<r.second.y; y++)
        {
            auto&c = l.get_lgt(x, y, 0);
            if (max_light < c.g)
                max_light = c.g;
        }

    was_light = was_light*(1-adapt_lvl) + max_light*adapt_lvl;
    if (was_light < min_level)
        was_light = min_level;
    float light_mod = brightness / was_light;
    for (int x = r.first.x; x<r.second.x; x++)
        for (int y = r.first.y; y<r.second.y; y++)
        {
            auto&c = l.get_lgt(x, y, 0);
            c *= light_mod;
        }
}
void lightingEngineViewscreen::calculate()
{
    auto& lmap = buffers.light;
    if(lmap.size()!=myRenderer->lightGrid.size())//TODO: multilevel render here
    {
        reinit();
        myRenderer->invalidate();//needs a lock?
    }
    rect2d vp=getMapViewport();
    float levelDim = cfg.levelDim;
    const rgbf dim(levelDim,levelDim,levelDim);

    lmap.assign(lmap.size(),rgbf(1,1,1));//clear to full bright
    buffers.emitters.assign(buffers.emitters.size(),rgbf(0,0,0));
    buffers.occlusion.assign(buffers.occlusion.size(), rgbf(1, 1, 1));
    for(int i=vp.first.x;i<vp.second.x;i++)
    for(int j=vp.first.y;j<vp.second.y;j++)
    {
        buffers.get_lgt(i,j,0)=dim; //dim the "map" portion
    }

    int window_x = *df::global::window_x;
    int window_y = *df::global::window_y;
    int window_z = *df::global::window_z;

    int render_w = vp.second.x - vp.first.x;
    int render_h = vp.second.y - vp.first.y;
    int render_d = 1;

    int render_offx = -window_x+1;
    int render_offy = -window_y+1;
    int render_offz = -window_z;

    calculate_sun(cfg, buffers, cfg.dayHour, 0, 0, render_offx, render_offy, render_offz, window_x, window_y, window_z, render_w, render_h, render_d);
    calculate_occlusion_and_emitters(cfg, buffers, render_offx, render_offy, render_offz, window_x, window_y, window_z, render_w, render_h, render_d);
    calculate_light(cfg, buffers, render_offx, render_offy, render_offz, window_x, window_y, window_z, render_w, render_h, render_d);
    
    if (cfg.adapt_speed > 0)
    {
        //transform to ciexyz
        rgb_to_cie(buffers,vp);
        //light adapt
        light_adapt(buffers,vp,cfg.adapt_speed,cfg.levelDim,cfg.adapt_brightness, was_light);
        //transform to rgb, clamping
        cie_to_rgb(buffers,vp);
        clamp_light(buffers,vp);
    }
}

void add_light(rgbf& light_sum, rgbf& occlusion_prod, const rgbf& occlusion, const rgbf& emitter, int dx, int dy)
{
    float part_of_cell = 1;

    if (dx*dx + dy*dy == 2)
        part_of_cell = RootTwo;

    occlusion_prod *= occlusion.pow(part_of_cell);
    light_sum += occlusion_prod*emitter*part_of_cell;
};
const float occlusion_eps = 0.000000001;
void gather_light_line(light_buffers& buffers,rgbf& light_sum, rgbf occlusion_prod,int x0, int y0, int x1, int y1,int current_z,
    int offx, int offy, int offz
)
{
    int dx = abs(x1 - x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0<y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */
    int rdx = 0;
    int rdy = 0;
    for (;;) {  /* loop */
        if (rdx != 0 || rdy != 0) //dirty hack to skip occlusion on the first tile.
        {
            int buf_x = x0 + offx;
            int buf_y = y0 + offy;
            int buf_z = current_z + offz;
            if (!buffers.coord_inside(buf_x, buf_y, buf_z))
                return;

            const rgbf& e_k = buffers.get_emt(buf_x, buf_y, buf_z);
            const rgbf& a_k = buffers.get_occ(buf_x, buf_y, buf_z);
            add_light(light_sum, occlusion_prod, a_k, e_k, rdx, rdy);

            if (occlusion_prod.dot(occlusion_prod)<occlusion_eps)
                return;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        rdx = rdy = 0;
        if (e2 >= dy) { err += dy; x0 += sx; rdx = sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; rdy = sy; } /* e_xy+e_y < 0 */
    }
    return;
}

void calculate_light(const light_config& config,light_buffers& buffers,int offx,int offy,int offz,int x,int y,int z,int w,int h,int d)
{
    const float max_radius = config.max_radius;
    

    for (int current_z = z; current_z < z + d; current_z++)
    {
        for (int tx = x; tx < x + w; tx++)
            for (int ty = y; ty < y + h; ty++)
            {
                //const auto& e = buffers.get_emt(tx + offx, ty + offy, current_z + offz);
                if (!buffers.coord_inside(tx + offx, ty + offy, current_z + offz)) //FIXME: this should not be a problem as we should only process VALID coords...
                    continue;
                auto& l = buffers.get_lgt(tx + offx, ty + offy, current_z + offz); //because we will multithread, we can't write to multiple locations, so we "gather" light from around us
                l = rgbf(0, 0,0);//final sum of light
                
                /*
                    Light collection works like this: 
                        start from end and if your lights are (L_1,L_2,...,L_n) where L_1 is furthest from "camera" (l in our case) and transclusency coofs are (a_1,a_2,a_3,...,a_n)
                        you collect this amount of light l=L_1*a_1*a_2*a_3*...*a_n+L_2*a_2*...*a_n+...+L_n*a_n
                        however in this for we can't abort early if a_n==0 (e.g. we are in wall) so after little mathe-magic:
                        l=a_n*(L_n+a_{n-1}*(L_{n-1}+...+a_1*L1))

                        and now we can collect a_n*...*a_k obstruction in one var and light in other
                */

                //we flip numbering because reasons...
                /* wall hack here. We skip the first tile (for occlusion). The gather_line things will include it in some way

                
                const rgbf& a0 = buffers.get_occ(tx + offx, ty + offy, current_z + offz);
                if (a0.dot(a0) < occlusion_eps)
                {
                    continue; //no light in this cell. Might need wall hack though...
                }
                l += a0*e0;//special case in 0,0
                //add_light(l, occlusion, a0, e0, 0, 0);
                */
                const rgbf& e0 = buffers.get_emt(tx + offx, ty + offy, current_z + offz);
                l += e0;

                rgbf occlusion(1, 1, 1);//product of all occlusions
                float count_hits = 0;
                //SQUARE LIGHT
                for (int dist = 0; dist <= max_radius; dist++)
                {
                    count_hits += 8;
                    gather_light_line(buffers,l,occlusion,tx,ty,tx + max_radius, ty +       dist,current_z,offx,offy,offz); /*   I.1 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx +       dist, ty + max_radius,current_z,offx,offy,offz); /*   I.2 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx + max_radius, ty -       dist,current_z,offx,offy,offz); /*   II.1 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx +       dist, ty - max_radius,current_z,offx,offy,offz); /*   II.2 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx - max_radius, ty -       dist,current_z,offx,offy,offz); /*   III.1 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx -       dist, ty - max_radius,current_z,offx,offy,offz); /*   III.2 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx - max_radius, ty +       dist,current_z,offx,offy,offz); /*   IV.1 Quadrant */
                    gather_light_line(buffers,l,occlusion,tx,ty,tx -       dist, ty + max_radius,current_z,offx,offy,offz); /*   IV.2 Quadrant */
                }
                //we norm the ligth somewhat...
                l *= (1/count_hits);
            }
    }
}
void lightingEngineViewscreen::updateWindow()
{
    tthread::lock_guard<tthread::fast_mutex> guard(myRenderer->dataMutex);
    if(buffers.light.size()!=myRenderer->lightGrid.size())
    {
        reinit();
        myRenderer->invalidate();
        return;
    }

    bool isAdventure=(*df::global::gametype==df::game_type::ADVENTURE_ARENA)||
        (*df::global::gametype==df::game_type::ADVENTURE_MAIN);
    if(isAdventure)
    {
        fixAdvMode(cfg.adv_mode);
    }

    if(doDebug==1)
        std::swap(buffers.occlusion,myRenderer->lightGrid);
    else if(doDebug==2)
        std::swap(buffers.emitters, myRenderer->lightGrid);
    else
        std::swap(buffers.light,myRenderer->lightGrid);

    rect2d vp=getMapViewport();

    myRenderer->invalidateRect(vp.first.x,vp.first.y,vp.second.x-vp.first.x,vp.second.y-vp.first.y);
}
void lightingEngineViewscreen::preRender()
{

}
void lightingEngineViewscreen::fixAdvMode(int mode)
{

    MapExtras::MapCache mc;
    const rgbf dim(cfg.levelDim, cfg.levelDim, cfg.levelDim);
    rect2d vp=getMapViewport();
    int window_x=*df::global::window_x;
    int window_y=*df::global::window_y;
    int window_z=*df::global::window_z;
    coord2d vpSize=rect_size(vp);
    //mode 0-> make dark non-visible parts
    if(mode==0)
    {
        for(int x=vp.first.x;x<vp.second.x;x++)
            for(int y=vp.first.y;y<vp.second.y;y++)
            {
                df::tile_designation d=mc.designationAt(DFCoord(window_x+x,window_y+y,window_z));
                if(d.bits.pile!=1)
                {
                    buffers.get_lgt(x,y,0)=dim;
                }
            }
    }
    //mode 1-> make everything visible, let the lighting hide stuff
    else if(mode==1)
    {
        for(int x=vp.first.x;x<vp.second.x;x++)
            for(int y=vp.first.y;y<vp.second.y;y++)
            {
                df::tile_designation d=mc.designationAt(DFCoord(window_x+x,window_y+y,window_z));
                d.bits.dig=df::tile_dig_designation::Default;//TODO add union and flag there
                d.bits.hidden=0;
                d.bits.pile = 1;
                mc.setDesignationAt(DFCoord(window_x+x,window_y+y,window_z),d);
            }
    }
}

/*

void light_job_list::reset()
{
    current_job = 0;
}

bool light_job_list::has_jobs()
{
    return current_job<all_jobs.size();
}

light_job light_job_list::get_job()
{
    int my_job_id = current_job.fetch_add(1);
    if (my_job_id >= all_jobs.size())
    {
        light_job invalid_job;
        invalid_job.type = light_job_type::none;
        return invalid_job;
    }
    return all_jobs[my_job_id];
}
*/