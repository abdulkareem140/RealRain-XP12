#pragma once

#include <string>
#include <vector>

#include "XPLMDataAccess.h"
#include "XPLMDisplay.h"

struct Config;
class SimData;
class StreakControl;

// Floating window that shows live sim values, probe datarefs and the state of
// the streak overrides. Text only, drawn with XPLMDrawString.
class DebugWindow
{
public:
    void Create(const Config* cfg, const SimData* sim, const StreakControl* streaks);
    void Destroy();
    void SetVisible(bool visible);
    bool IsVisible() const;
    void SetProbes(const std::vector<std::string>& names);

private:
    static void DrawCb(XPLMWindowID id, void* refcon);
    static int MouseCb(XPLMWindowID, int, int, XPLMMouseStatus, void*) { return 1; }
    static void KeyCb(XPLMWindowID, char, XPLMKeyFlags, char, void*, int) {}
    static XPLMCursorStatus CursorCb(XPLMWindowID, int, int, void*) { return xplm_CursorDefault; }
    static int WheelCb(XPLMWindowID, int, int, int, int, void*) { return 0; }
    void Draw();

    XPLMWindowID window_ = nullptr;
    const Config* cfg_ = nullptr;
    const SimData* sim_ = nullptr;
    const StreakControl* streaks_ = nullptr;

    struct Probe { std::string name; XPLMDataRef ref = nullptr; };
    std::vector<Probe> probes_;
};
