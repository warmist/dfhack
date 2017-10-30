#ifndef RENDERER_LIGHT_INCLUDED
#define RENDERER_LIGHT_INCLUDED
#include "renderer_opengl.hpp"
#include "Types.h"
#include <tuple>
#include <stack>
#include <memory>
#include <atomic>

#include "map_reading.hpp"


bool isInRect(const df::coord2d& pos,const DFHack::rect2d& rect);
struct renderer_light : public renderer_wrap {
private:
    void colorizeTile(int x,int y)
    {
        const int tile = x*(df::global::gps->dimy) + y;
        old_opengl* p=reinterpret_cast<old_opengl*>(parent);
        float *fg = p->fg + tile * 4 * 6;
        float *bg = p->bg + tile * 4 * 6;
        float *tex = p->tex + tile * 2 * 6;
        rgbf light=lightGrid[tile];

        for (int i = 0; i < 6; i++) { //oh how sse would do wonders here, or shaders...
            *(fg++) *= light.r;
            *(fg++) *= light.g;
            *(fg++) *= light.b;
            *(fg++) = 1;

            *(bg++) *= light.r;
            *(bg++) *= light.g;
            *(bg++) *= light.b;
            *(bg++) = 1;
        }
    }
    void reinitLightGrid(int w,int h)
    {
        tthread::lock_guard<tthread::fast_mutex> guard(dataMutex);
        lightGrid.resize(w*h,rgbf(1,1,1));
    }
    void reinitLightGrid()
    {
        reinitLightGrid(df::global::gps->dimy,df::global::gps->dimx);
    }

public:
    tthread::fast_mutex dataMutex;
    std::vector<rgbf> lightGrid;
    renderer_light(renderer* parent):renderer_wrap(parent)
    {
        reinitLightGrid();
    }
    virtual void update_tile(int32_t x, int32_t y) {
        renderer_wrap::update_tile(x,y);
        tthread::lock_guard<tthread::fast_mutex> guard(dataMutex);
        colorizeTile(x,y);
    };
    virtual void update_all() {
        renderer_wrap::update_all();
        tthread::lock_guard<tthread::fast_mutex> guard(dataMutex);
        for (int x = 0; x < df::global::gps->dimx; x++)
            for (int y = 0; y < df::global::gps->dimy; y++)
                colorizeTile(x,y);
    };
    virtual void grid_resize(int32_t w, int32_t h) {
        renderer_wrap::grid_resize(w,h);
        reinitLightGrid(w,h);
    };
    virtual void resize(int32_t w, int32_t h) {
        renderer_wrap::resize(w,h);
        reinitLightGrid();
    }
    virtual void set_fullscreen()
    {
        renderer_wrap::set_fullscreen();
        reinitLightGrid();
    }
    virtual void zoom(df::zoom_commands z)
    {
        renderer_wrap::zoom(z);
        reinitLightGrid();
    }
};


class lightingEngine
{
    
public:
    light_config cfg;

    lightingEngine(renderer_light* target):myRenderer(target){}
    virtual ~lightingEngine(){}
    virtual void reinit()=0;
    virtual void calculate()=0;

    virtual void updateWindow()=0;
    virtual void preRender()=0;

    void loadSettings()
    {
        cfg.load_settings();
    }
    virtual void clear()=0;

    virtual void setHour(float h)
    {
        cfg.dayHour = h;
    }
    virtual void debug(int mode)=0;
protected:
    renderer_light* myRenderer;
    light_buffers buffers;
};

class lightingEngineViewscreen;
/*
enum class light_job_type
{
    none, //do nothing. When don't have any jobs left
    sun, //the sun rays. Does multiple z indexes almost always
    general, //occlusion and emitters. Main work here
    light, //light propagation
};
struct light_job
{
    int x, y, z; //world coordinates
    int w, h, d; //area
    int offx, offy, offz; //offset into buffers

    light_job_type type;
};
struct light_job_list
{
    std::vector<light_job> all_jobs;
    std::atomic<int> current_job;

    void reset();
    bool has_jobs();
    light_job get_job();
};
*/
void calculate_light(const light_config& cfg, light_buffers& buffers, int offx, int offy, int offz,
    int x, int y, int z, int w, int h, int d = 1);
class lightingEngineViewscreen:public lightingEngine
{
public:
    lightingEngineViewscreen(renderer_light* target);
    ~lightingEngineViewscreen();
    void reinit();
    void calculate();

    void updateWindow();
    void preRender();
    void clear();

    void debug(int mode){doDebug=mode;};
private:
    void fixAdvMode(int mode);
    float was_light;
    //light_job_list job_list;
    //std::vector<std::unique_ptr<tthread::thread>> threads;
    //void generate_jobs();
    //void shutdown_threads();
private:
    int doDebug;
};
#endif
