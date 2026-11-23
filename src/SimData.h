#pragma once

#include <string>
#include <vector>

#include "XPLMDataAccess.h"

// One logical sim value. Several dataref names may be tried in order, because
// XP12 moved some weather datarefs and older names may still exist as aliases.
struct SimValue
{
    const char* label;                    // shown in the debug window
    const char* units;
    std::vector<const char*> candidates;  // dataref names, tried in order

    XPLMDataRef ref = nullptr;
    std::string resolvedName;             // which candidate was found
    bool isInt = false;
    bool isDouble = false;
    double value = 0.0;
};

// Everything RealRain reads from the sim each frame.
class SimData
{
public:
    enum Id {
        PrecipRatio,
        RainPercentLegacy,
        TrueAirspeed,
        IndicatedAirspeed,
        GroundSpeed,
        ElevationMsl,
        HeightAgl,
        OutsideAirTemp,
        WindX, WindY, WindZ,
        Pitch, Roll, Heading,
        LocalVx, LocalVy, LocalVz,
        LocalX, LocalY, LocalZ,
        EyeX, EyeY, EyeZ,
        ViewExternal,
        Count
    };

    SimData();
    void Resolve();   // look up datarefs (call after aircraft load too)
    void Update();    // read current values

    double Get(Id id) const { return values_[id].value; }
    bool Has(Id id) const { return values_[id].ref != nullptr; }
    const std::vector<SimValue>& All() const { return values_; }

private:
    std::vector<SimValue> values_;
};

// Reads any dataref (int/float/double) as a double. Returns false if null.
bool ReadAsDouble(XPLMDataRef ref, double& out);
