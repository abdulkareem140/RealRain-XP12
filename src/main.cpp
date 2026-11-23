// RealRain XP12 - plugin entry points.
//
// Prototype stage: diagnostics only. Answers three questions before we build
// the real droplet renderer:
//   1. Which weather / flight datarefs exist and what do they read?
//   2. Can X-Plane's built-in rain streaks be suppressed via datarefs?
//   3. Can we draw airframe-fixed geometry (the windshield) under Vulkan/Metal?

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "Config.h"
#include "DatarefScan.h"
#include "DebugWindow.h"
#include "Log.h"
#include "SimData.h"
#include "StreakControl.h"
#include "TestQuad.h"

#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

namespace {

struct Plugin
{
    std::string configPath;
    Config config;
    SimData sim;
    StreakControl streaks;
    DebugWindow window;
    TestQuad quad;

    XPLMFlightLoopID flightLoop = nullptr;
    XPLMMenuID menu = nullptr;
    int menuParentItem = -1;
};

std::unique_ptr<Plugin> g;

enum MenuItem : intptr_t {
    kMenuDebugWindow,
    kMenuTestQuad,
    kMenuStreaks,
    kMenuScan,
    kMenuReload,
};
int gMenuIndex[5] = {-1, -1, -1, -1, -1};

// <X-Plane>/Resources/plugins/RealRain/RealRain.ini
std::string FindConfigPath()
{
    char path[2048] = {};
    XPLMGetPluginInfo(XPLMGetMyID(), nullptr, path, nullptr, nullptr);
    // path = .../RealRain/win_x64/RealRain.xpl  -> strip two components.
    std::string p(path);
    for (int i = 0; i < 2; ++i) {
        size_t cut = p.find_last_of("/\\");
        if (cut == std::string::npos) break;
        p.erase(cut);
    }
    return p + XPLMGetDirectorySeparator() + "RealRain.ini";
}

void UpdateMenuChecks()
{
    if (!g || !g->menu) return;
    auto check = [](MenuItem item, bool on) {
        XPLMCheckMenuItem(g->menu, gMenuIndex[item], on ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    };
    check(kMenuDebugWindow, g->window.IsVisible());
    check(kMenuTestQuad, g->config.testQuadEnabled);
    check(kMenuStreaks, g->streaks.IsApplied());
}

void ApplyConfig()
{
    g->streaks.SetOverrides(g->config.overrides);
    if (g->config.streakOverridesEnabled) g->streaks.Apply();
    g->window.SetProbes(g->config.probes);
    g->window.SetVisible(g->config.showDebugWindow);
    UpdateMenuChecks();
}

void ReloadConfig()
{
    g->streaks.Restore();
    g->config = LoadConfig(g->configPath);
    g->sim.Resolve();
    ApplyConfig();
}

void MenuHandler(void* /*menuRef*/, void* itemRef)
{
    switch (static_cast<MenuItem>(reinterpret_cast<intptr_t>(itemRef))) {
    case kMenuDebugWindow:
        g->window.SetVisible(!g->window.IsVisible());
        break;
    case kMenuTestQuad:
        g->config.testQuadEnabled = !g->config.testQuadEnabled;
        break;
    case kMenuStreaks:
        if (g->streaks.IsApplied()) g->streaks.Restore(); else g->streaks.Apply();
        break;
    case kMenuScan: {
        int n = ScanDatarefs(g->config.scanTerms);
        LogMsg("scan finished: %d matches written to Log.txt", n);
        break;
    }
    case kMenuReload:
        ReloadConfig();
        break;
    }
    UpdateMenuChecks();
}

float FlightLoop(float, float, int, void*)
{
    g->sim.Update();
    return -1.0f;   // every frame
}

void* ItemRef(MenuItem m) { return reinterpret_cast<void*>(static_cast<intptr_t>(m)); }

} // namespace

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    std::strcpy(outName, "RealRain XP12");
    std::strcpy(outSig, "realrain.xp12.plugin");
    std::strcpy(outDesc, "Realistic rain on the windshield for X-Plane 12.");

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    g = std::make_unique<Plugin>();
    g->configPath = FindConfigPath();
    g->config = LoadConfig(g->configPath);

    LogMsg("version %s started", REALRAIN_VERSION);
    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    g.reset();
    LogMsg("stopped");
}

PLUGIN_API int XPluginEnable(void)
{
    g->sim.Resolve();

    XPLMCreateFlightLoop_t fl{};
    fl.structSize = sizeof(fl);
    fl.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    fl.callbackFunc = &FlightLoop;
    fl.refcon = nullptr;
    g->flightLoop = XPLMCreateFlightLoop(&fl);
    XPLMScheduleFlightLoop(g->flightLoop, -1.0f, 1);

    g->menuParentItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "RealRain", nullptr, 0);
    g->menu = XPLMCreateMenu("RealRain", XPLMFindPluginsMenu(), g->menuParentItem, &MenuHandler, nullptr);
    gMenuIndex[kMenuDebugWindow] = XPLMAppendMenuItem(g->menu, "Debug window", ItemRef(kMenuDebugWindow), 0);
    gMenuIndex[kMenuTestQuad]    = XPLMAppendMenuItem(g->menu, "Windshield test quad", ItemRef(kMenuTestQuad), 0);
    gMenuIndex[kMenuStreaks]     = XPLMAppendMenuItem(g->menu, "Apply streak overrides", ItemRef(kMenuStreaks), 0);
    XPLMAppendMenuSeparator(g->menu);
    gMenuIndex[kMenuScan]        = XPLMAppendMenuItem(g->menu, "Scan rain datarefs to Log.txt", ItemRef(kMenuScan), 0);
    gMenuIndex[kMenuReload]      = XPLMAppendMenuItem(g->menu, "Reload RealRain.ini", ItemRef(kMenuReload), 0);

    g->window.Create(&g->config, &g->sim, &g->streaks);
    g->quad.Register(&g->config, &g->sim);
    ApplyConfig();

    LogMsg("enabled (config: %s)", g->configPath.c_str());
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    // Always hand X-Plane its original settings back.
    g->streaks.Restore();
    g->quad.Unregister();
    g->window.Destroy();

    if (g->menu) XPLMDestroyMenu(g->menu);
    if (g->menuParentItem >= 0) XPLMRemoveMenuItem(XPLMFindPluginsMenu(), g->menuParentItem);
    g->menu = nullptr;
    g->menuParentItem = -1;

    if (g->flightLoop) XPLMDestroyFlightLoop(g->flightLoop);
    g->flightLoop = nullptr;
    LogMsg("disabled");
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID /*from*/, int msg, void* param)
{
    // Aircraft datarefs (eye point etc.) can change when the user's plane loads.
    if (msg == XPLM_MSG_PLANE_LOADED && reinterpret_cast<intptr_t>(param) == 0 && g)
        g->sim.Resolve();
}
