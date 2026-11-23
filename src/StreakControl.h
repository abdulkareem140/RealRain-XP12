#pragma once

#include <string>
#include <utility>
#include <vector>

#include "XPLMDataAccess.h"

// Forces a list of (usually private / art-control) datarefs to given values
// and restores the originals when turned off. This is how we try to suppress
// X-Plane's built-in rain streaks. The dataref names are NOT known in advance:
// find them with "Scan datarefs" and list them in RealRain.ini.
class StreakControl
{
public:
    void SetOverrides(const std::vector<std::pair<std::string, double>>& list);
    void Apply();      // save originals, write override values
    void Restore();    // write originals back
    bool IsApplied() const { return applied_; }

    struct Entry {
        std::string name;
        double target = 0.0;
        double original = 0.0;
        XPLMDataRef ref = nullptr;
        bool writable = false;
    };
    const std::vector<Entry>& Entries() const { return entries_; }

private:
    std::vector<Entry> entries_;
    bool applied_ = false;
};
