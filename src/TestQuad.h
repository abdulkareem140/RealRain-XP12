#pragma once

struct Config;
class SimData;

// Prototype step 3: proves we can draw geometry fixed to the airframe (where the
// windshield will be) from a plugin under X-Plane 12's Vulkan/Metal renderer.
// Draws a translucent rectangle in front of the pilot eye point, in aircraft
// coordinates, during xplm_Phase_Modern3D.
class TestQuad
{
public:
    void Register(const Config* cfg, const SimData* sim);
    void Unregister();

private:
    static int DrawCallback(int phase, int isBefore, void* refcon);
    void Draw();

    const Config* cfg_ = nullptr;
    const SimData* sim_ = nullptr;
    bool registered_ = false;
};
