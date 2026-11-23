#include "Config.h"

#include <cctype>
#include <cstdlib>
#include <fstream>

#include "DefaultIni.h"
#include "Log.h"

namespace {

std::string Trim(const std::string& s)
{
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

bool ParseBool(const std::string& v)
{
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

std::vector<std::string> SplitList(const std::string& v)
{
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= v.size()) {
        size_t comma = v.find(',', start);
        if (comma == std::string::npos) comma = v.size();
        std::string item = Trim(v.substr(start, comma - start));
        if (!item.empty()) out.push_back(item);
        start = comma + 1;
    }
    return out;
}

} // namespace

Config LoadConfig(const std::string& path)
{
    Config cfg;
    std::ifstream in(path);
    if (!in) {
        // First run (or deleted): write the default file, then read it.
        std::ofstream out(path, std::ios::binary);
        if (out) {
            out << kDefaultIni;
            out.close();
            LogMsg("wrote default config to %s", path.c_str());
            in.open(path);
        }
        if (!in) {
            LogMsg("config not found at %s and could not be created, using defaults", path.c_str());
            return cfg;
        }
    }
    cfg.loadedFrom = path;

    std::string raw;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        size_t hash = raw.find('#');
        if (hash != std::string::npos) raw.erase(hash);
        std::string line = Trim(raw);
        if (line.empty()) continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            LogMsg("config line %d ignored (no '='): %s", lineNo, line.c_str());
            continue;
        }
        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));

        if (key == "show_debug_window")            cfg.showDebugWindow = ParseBool(val);
        else if (key == "test_quad")               cfg.testQuadEnabled = ParseBool(val);
        else if (key == "test_quad_depth_test")    cfg.testQuadDepthTest = ParseBool(val);
        else if (key == "streak_overrides")        cfg.streakOverridesEnabled = ParseBool(val);
        else if (key == "test_quad_offset_x")      cfg.quadOffsetX = std::strtof(val.c_str(), nullptr);
        else if (key == "test_quad_offset_y")      cfg.quadOffsetY = std::strtof(val.c_str(), nullptr);
        else if (key == "test_quad_offset_z")      cfg.quadOffsetZ = std::strtof(val.c_str(), nullptr);
        else if (key == "test_quad_width")         cfg.quadWidth = std::strtof(val.c_str(), nullptr);
        else if (key == "test_quad_height")        cfg.quadHeight = std::strtof(val.c_str(), nullptr);
        else if (key == "scan_terms")              cfg.scanTerms = SplitList(val);
        else if (key == "probe")                   cfg.probes.push_back(val);
        else if (key.rfind("override ", 0) == 0) {
            // "override sim/some/dataref = 0.0"
            std::string name = Trim(key.substr(9));
            if (!name.empty()) cfg.overrides.emplace_back(name, std::strtod(val.c_str(), nullptr));
        }
        else LogMsg("config line %d: unknown key '%s'", lineNo, key.c_str());
    }

    LogMsg("config loaded from %s (%zu probes, %zu overrides)",
           path.c_str(), cfg.probes.size(), cfg.overrides.size());
    return cfg;
}
