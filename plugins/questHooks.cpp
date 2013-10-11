#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <VTableInterpose.h>

#include "df/adv_task.h"
#include "df/task_kill_nemesisst.h"
#include "df/task_seek_nemesisst.h"

#include "MiscUtils.h"
#include "LuaTools.h"

using namespace DFHack;
using namespace df::enums;


DFHACK_PLUGIN("questHooks");

static void handleGetStatus(color_ostream &out,df::task_kill_nemesisst*,int32_t*){};
static void handleGetDescription(color_ostream &out,df::task_kill_nemesisst*,std::string*){};
static void handleOnKill(color_ostream &out,df::task_kill_nemesisst*,int32_t,bool*){};
static void handleUpdateTargetPos(color_ostream &out,df::task_kill_nemesisst*,bool*){};
static void handleGetGoalSite(color_ostream &out,df::task_kill_nemesisst*,int32_t*){};
static void handleGetTargetHistFig(color_ostream &out,df::task_kill_nemesisst*,int32_t*){};
DEFINE_LUA_EVENT_2(onGetStatus, handleGetStatus, df::task_kill_nemesisst*,int32_t*);
DEFINE_LUA_EVENT_2(onGetDescription, handleGetDescription, df::task_kill_nemesisst*,std::string*);
DEFINE_LUA_EVENT_3(onKillEvent, handleOnKill, df::task_kill_nemesisst*,int32_t,bool*);
DEFINE_LUA_EVENT_2(onUpdateTargetPos,handleUpdateTargetPos,df::task_kill_nemesisst*,bool*);
DEFINE_LUA_EVENT_2(onGetGoalSite,handleGetGoalSite,df::task_kill_nemesisst*,int32_t*);
DEFINE_LUA_EVENT_2(onGetTargetHistFig,handleGetTargetHistFig,df::task_kill_nemesisst*,int32_t*);
DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onGetStatus),
    DFHACK_LUA_EVENT(onGetDescription),
    DFHACK_LUA_EVENT(onKillEvent),
    DFHACK_LUA_EVENT(onUpdateTargetPos),
    DFHACK_LUA_EVENT(onGetGoalSite),
    DFHACK_LUA_EVENT(onGetTargetHistFig),
    DFHACK_LUA_END
};


struct adv_taks_kill_hook: df::task_kill_nemesisst{
    typedef df::task_kill_nemesisst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(int32_t,getStatus,())
    {
        int32_t origRet=INTERPOSE_NEXT(getStatus)();
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onGetStatus(out,this,&origRet);

        return origRet;
    }
    DEFINE_VMETHOD_INTERPOSE(void,getDescription,(std::string* buf))
    {
        INTERPOSE_NEXT(getDescription)(buf);
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onGetDescription(out,this,buf);
    }
    DEFINE_VMETHOD_INTERPOSE(void,onKill,(int32_t hist_fig))
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        bool call_native=true;
        onKillEvent(out,this,hist_fig,&call_native);
        if(call_native)
            INTERPOSE_NEXT(onKill)(hist_fig);
    }
    DEFINE_VMETHOD_INTERPOSE(void,updateTargetPos,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        bool call_native=true;
        onUpdateTargetPos(out,this,&call_native);
        if(call_native)
            INTERPOSE_NEXT(updateTargetPos)();
    }
    DEFINE_VMETHOD_INTERPOSE(int32_t,getGoalSite,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        int32_t origRet=INTERPOSE_NEXT(getGoalSite)();
        onGetGoalSite(out,this,&origRet);
        return origRet;
    }
    DEFINE_VMETHOD_INTERPOSE(int32_t,getTargetHistFig,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        int32_t origRet=INTERPOSE_NEXT(getTargetHistFig)();
        onGetTargetHistFig(out,this,&origRet);
        return origRet;
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(adv_taks_kill_hook,getStatus);
IMPLEMENT_VMETHOD_INTERPOSE(adv_taks_kill_hook,getDescription);
IMPLEMENT_VMETHOD_INTERPOSE(adv_taks_kill_hook,onKill);
IMPLEMENT_VMETHOD_INTERPOSE(adv_taks_kill_hook,updateTargetPos);
IMPLEMENT_VMETHOD_INTERPOSE(adv_taks_kill_hook,getTargetHistFig);
IMPLEMENT_VMETHOD_INTERPOSE(adv_taks_kill_hook,getGoalSite);
static void enable_hooks(bool enable)
{
    INTERPOSE_HOOK(adv_taks_kill_hook,getStatus).apply(enable);
    INTERPOSE_HOOK(adv_taks_kill_hook,getDescription).apply(enable);
    INTERPOSE_HOOK(adv_taks_kill_hook,onKill).apply(enable);
    INTERPOSE_HOOK(adv_taks_kill_hook,updateTargetPos).apply(enable);
    INTERPOSE_HOOK(adv_taks_kill_hook,getTargetHistFig).apply(enable);
    INTERPOSE_HOOK(adv_taks_kill_hook,getGoalSite).apply(enable);
}
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    enable_hooks(true);
    return CR_OK;
}
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    enable_hooks(false);
    return CR_OK;
}
