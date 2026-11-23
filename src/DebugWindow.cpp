#include "DebugWindow.h"

#include <cstdio>

#include "Config.h"
#include "SimData.h"
#include "StreakControl.h"
#include "XPLMGraphics.h"

namespace {

constexpr int kWidth = 520;
constexpr int kHeight = 560;
constexpr int kLine = 14;

float kWhite[3] = {1.0f, 1.0f, 1.0f};
float kGrey[3]  = {0.65f, 0.65f, 0.65f};
float kGreen[3] = {0.4f, 1.0f, 0.4f};
float kRed[3]   = {1.0f, 0.45f, 0.45f};
float kCyan[3]  = {0.4f, 0.85f, 1.0f};

} // namespace

void DebugWindow::Create(const Config* cfg, const SimData* sim, const StreakControl* streaks)
{
    cfg_ = cfg;
    sim_ = sim;
    streaks_ = streaks;
    if (window_) return;

    int l = 0, t = 0, r = 0, b = 0;
    XPLMGetScreenBoundsGlobal(&l, &t, &r, &b);

    XPLMCreateWindow_t p{};
    p.structSize = sizeof(p);
    p.left = l + 60;
    p.top = t - 120;
    p.right = p.left + kWidth;
    p.bottom = p.top - kHeight;
    p.visible = 1;
    p.drawWindowFunc = &DebugWindow::DrawCb;
    p.handleMouseClickFunc = &DebugWindow::MouseCb;
    p.handleRightClickFunc = &DebugWindow::MouseCb;
    p.handleKeyFunc = &DebugWindow::KeyCb;
    p.handleCursorFunc = &DebugWindow::CursorCb;
    p.handleMouseWheelFunc = &DebugWindow::WheelCb;
    p.refcon = this;
    p.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
    p.layer = xplm_WindowLayerFloatingWindows;

    window_ = XPLMCreateWindowEx(&p);
    XPLMSetWindowPositioningMode(window_, xplm_WindowPositionFree, -1);
    XPLMSetWindowResizingLimits(window_, 360, 240, 1200, 1400);
    XPLMSetWindowTitle(window_, "RealRain debug");
}

void DebugWindow::Destroy()
{
    if (window_) XPLMDestroyWindow(window_);
    window_ = nullptr;
}

void DebugWindow::SetVisible(bool visible)
{
    if (window_) XPLMSetWindowIsVisible(window_, visible ? 1 : 0);
}

bool DebugWindow::IsVisible() const
{
    return window_ && XPLMGetWindowIsVisible(window_);
}

void DebugWindow::SetProbes(const std::vector<std::string>& names)
{
    probes_.clear();
    for (const auto& n : names) probes_.push_back({n, XPLMFindDataRef(n.c_str())});
}

void DebugWindow::DrawCb(XPLMWindowID, void* refcon)
{
    static_cast<DebugWindow*>(refcon)->Draw();
}

void DebugWindow::Draw()
{
    XPLMSetGraphicsState(0, 0, 0, 0, 1, 0, 0);

    int l = 0, t = 0, r = 0, b = 0;
    XPLMGetWindowGeometry(window_, &l, &t, &r, &b);

    const int x = l + 10;
    int y = t - 20;
    char buf[512];

    auto line = [&](float* color, const char* text) {
        if (y < b + 4) return;
        XPLMDrawString(color, x, y, const_cast<char*>(text), nullptr, xplmFont_Basic);
        y -= kLine;
    };

    std::snprintf(buf, sizeof(buf), "RealRain %s  -  prototype diagnostics", REALRAIN_VERSION);
    line(kCyan, buf);
    y -= 4;

    line(kCyan, "Sim values");
    for (const SimValue& v : sim_->All()) {
        if (v.ref) {
            std::snprintf(buf, sizeof(buf), "  %-20s %10.3f %-4s  %s",
                          v.label, v.value, v.units, v.resolvedName.c_str());
            line(kWhite, buf);
        } else {
            std::snprintf(buf, sizeof(buf), "  %-20s   MISSING", v.label);
            line(kRed, buf);
        }
    }

    y -= 4;
    std::snprintf(buf, sizeof(buf), "Streak overrides: %s (%zu configured)",
                  streaks_->IsApplied() ? "APPLIED" : "off", streaks_->Entries().size());
    line(kCyan, buf);
    for (const auto& e : streaks_->Entries()) {
        if (!e.ref)
            std::snprintf(buf, sizeof(buf), "  %s  NOT FOUND", e.name.c_str());
        else if (!e.writable)
            std::snprintf(buf, sizeof(buf), "  %s  READ-ONLY", e.name.c_str());
        else
            std::snprintf(buf, sizeof(buf), "  %s  target %.3f  (orig %.3f)",
                          e.name.c_str(), e.target, e.original);
        line(e.writable ? kGreen : kRed, buf);
    }

    y -= 4;
    std::snprintf(buf, sizeof(buf), "Probes (%zu)", probes_.size());
    line(kCyan, buf);
    for (const Probe& p : probes_) {
        double v = 0.0;
        if (ReadAsDouble(p.ref, v)) {
            std::snprintf(buf, sizeof(buf), "  %s = %.4f%s", p.name.c_str(), v,
                          XPLMCanWriteDataRef(p.ref) ? "" : "  (ro)");
            line(kGreen, buf);
        } else {
            std::snprintf(buf, sizeof(buf), "  %s  %s", p.name.c_str(), p.ref ? "unreadable type" : "NOT FOUND");
            line(kRed, buf);
        }
    }

    y -= 4;
    std::snprintf(buf, sizeof(buf), "Test quad: %s   depth test: %s",
                  cfg_->testQuadEnabled ? "ON" : "off", cfg_->testQuadDepthTest ? "on" : "off");
    line(kGrey, buf);
    line(kGrey, "Plugins > RealRain for toggles, dataref scan and config reload.");
}
