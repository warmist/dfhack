#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "steam/steam_api.h"

using namespace DFHack;

DFHACK_PLUGIN("steam_support");

void test_controller()
{
    
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if(!SteamAPI_Init())
    {
        out.printerr("Steam failed to init.\n");
        return CR_FAILURE;
    }
    SteamController()->Init();
    return CR_OK;
}
DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    SteamAPI_RunCallbacks();
}
DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    SteamController()->Shutdown();
    SteamAPI_Shutdown();
    return CR_OK;
}

