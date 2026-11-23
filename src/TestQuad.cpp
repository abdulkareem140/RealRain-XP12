#include "TestQuad.h"

#if IBM
#include <windows.h>
#endif
#if APL
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "Config.h"
#include "Log.h"
#include "SimData.h"
#include "XPLMDataAccess.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"

namespace {

XPLMDataRef gWorldMatrix = nullptr;
XPLMDataRef gProjMatrix = nullptr;
XPLMDataRef gReverseZ = nullptr;

} // namespace

void TestQuad::Register(const Config* cfg, const SimData* sim)
{
    cfg_ = cfg;
    sim_ = sim;
    if (registered_) return;

    gWorldMatrix = XPLMFindDataRef("sim/graphics/view/world_matrix");
    gProjMatrix  = XPLMFindDataRef("sim/graphics/view/projection_matrix_3d");
    gReverseZ    = XPLMFindDataRef("sim/graphics/view/is_reverse_float_z");
    if (!gWorldMatrix || !gProjMatrix)
        LogMsg("test quad: view matrix datarefs missing; drawing will use whatever GL state X-Plane leaves");

    // Modern3D is the XP11.50+/XP12 3D phase that still allows plugin GL drawing.
    XPLMRegisterDrawCallback(&TestQuad::DrawCallback,
                             xplm_Phase_Modern3D, 0 /* after */, this);
    registered_ = true;
    LogMsg("test quad: draw callback registered (Modern3D)");
}

void TestQuad::Unregister()
{
    if (!registered_) return;
    XPLMUnregisterDrawCallback(&TestQuad::DrawCallback,
                               xplm_Phase_Modern3D, 0, this);
    registered_ = false;
}

int TestQuad::DrawCallback(int /*phase*/, int /*isBefore*/, void* refcon)
{
    auto* self = static_cast<TestQuad*>(refcon);
    if (self && self->cfg_ && self->cfg_->testQuadEnabled) self->Draw();
    return 1;
}

void TestQuad::Draw()
{
    const SimData& s = *sim_;
    const Config& c = *cfg_;

    // Corner positions in aircraft coordinates (x right, y up, z aft), meters,
    // relative to the CG. Centered on pilot eye point + configured offset.
    const float cx = static_cast<float>(s.Get(SimData::EyeX)) + c.quadOffsetX;
    const float cy = static_cast<float>(s.Get(SimData::EyeY)) + c.quadOffsetY;
    const float cz = static_cast<float>(s.Get(SimData::EyeZ)) + c.quadOffsetZ;
    const float hw = c.quadWidth * 0.5f;
    const float hh = c.quadHeight * 0.5f;

    // Alpha blending on; depth test optional (off = always visible).
    XPLMSetGraphicsState(0, 0, 0, 0, 1, c.testQuadDepthTest ? 1 : 0, 0);
    if (c.testQuadDepthTest && gReverseZ && XPLMGetDatai(gReverseZ))
        glDepthFunc(GL_GEQUAL);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    if (gProjMatrix) {
        float proj[16];
        XPLMGetDatavf(gProjMatrix, proj, 0, 16);
        glLoadMatrixf(proj);
    }
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (gWorldMatrix) {
        float world[16];
        XPLMGetDatavf(gWorldMatrix, world, 0, 16);
        glLoadMatrixf(world);
    }

    // Aircraft frame -> OpenGL local frame.
    glTranslatef(static_cast<float>(s.Get(SimData::LocalX)),
                 static_cast<float>(s.Get(SimData::LocalY)),
                 static_cast<float>(s.Get(SimData::LocalZ)));
    glRotatef(-static_cast<float>(s.Get(SimData::Heading)), 0.0f, 1.0f, 0.0f);
    glRotatef( static_cast<float>(s.Get(SimData::Pitch)),   1.0f, 0.0f, 0.0f);
    glRotatef(-static_cast<float>(s.Get(SimData::Roll)),    0.0f, 0.0f, 1.0f);

    // Translucent fill.
    glColor4f(0.2f, 0.6f, 1.0f, 0.35f);
    glBegin(GL_QUADS);
    glVertex3f(cx - hw, cy - hh, cz);
    glVertex3f(cx + hw, cy - hh, cz);
    glVertex3f(cx + hw, cy + hh, cz);
    glVertex3f(cx - hw, cy + hh, cz);
    glEnd();

    // Outline + cross so orientation is obvious.
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(cx - hw, cy - hh, cz);
    glVertex3f(cx + hw, cy - hh, cz);
    glVertex3f(cx + hw, cy + hh, cz);
    glVertex3f(cx - hw, cy + hh, cz);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(cx - hw, cy - hh, cz); glVertex3f(cx + hw, cy + hh, cz);
    glVertex3f(cx - hw, cy + hh, cz); glVertex3f(cx + hw, cy - hh, cz);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (c.testQuadDepthTest && gReverseZ && XPLMGetDatai(gReverseZ))
        glDepthFunc(GL_LEQUAL);
}
