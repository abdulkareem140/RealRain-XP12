#pragma once

#include <string>
#include <utility>
#include <vector>

// Settings read from RealRain.ini (next to the platform folder).
struct Config
{
    // Debug / prototype switches
    bool showDebugWindow = true;
    bool testQuadEnabled = false;
    bool testQuadDepthTest = false;   // false = always drawn on top (easiest to see)
    bool streakOverridesEnabled = false;

    // Test quad placement, meters, relative to the pilot eye point in aircraft
    // coordinates (x = right, y = up, z = aft; so forward is negative z).
    float quadOffsetX = 0.0f;
    float quadOffsetY = 0.0f;
    float quadOffsetZ = -0.9f;
    float quadWidth = 0.8f;
    float quadHeight = 0.4f;

    // Substrings used by "Scan datarefs" (case-insensitive).
    std::vector<std::string> scanTerms = {"rain", "precip", "streak", "drop", "wet", "wiper"};

    // Datarefs whose value is printed in the debug window (existence probe).
    std::vector<std::string> probes;

    // dataref name -> value to force while streak overrides are enabled.
    std::vector<std::pair<std::string, double>> overrides;

    std::string loadedFrom;           // path actually read (empty if defaults)
};

// Loads the config. Missing file is not an error: defaults are kept.
Config LoadConfig(const std::string& path);
