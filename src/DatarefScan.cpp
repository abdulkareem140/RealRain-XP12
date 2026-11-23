#include "DatarefScan.h"

#include <algorithm>
#include <cctype>

#include "Log.h"
#include "SimData.h"
#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"

namespace {

std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string TypeName(XPLMDataTypeID t)
{
    std::string s;
    if (t & xplmType_Int)        s += "int ";
    if (t & xplmType_Float)      s += "float ";
    if (t & xplmType_Double)     s += "double ";
    if (t & xplmType_FloatArray) s += "float[] ";
    if (t & xplmType_IntArray)   s += "int[] ";
    if (t & xplmType_Data)       s += "data ";
    if (s.empty()) s = "? ";
    s.pop_back();
    return s;
}

} // namespace

int ScanDatarefs(const std::vector<std::string>& terms)
{
    std::vector<std::string> lowerTerms;
    for (const auto& t : terms) lowerTerms.push_back(Lower(t));

    const int total = XPLMCountDataRefs();
    LogMsg("scan: %d datarefs registered, searching for %zu term(s)", total, lowerTerms.size());

    constexpr int kBatch = 512;
    std::vector<XPLMDataRef> batch(kBatch);
    int matches = 0;

    for (int offset = 0; offset < total; offset += kBatch) {
        const int n = std::min(kBatch, total - offset);
        XPLMGetDataRefsByIndex(offset, n, batch.data());

        for (int i = 0; i < n; ++i) {
            XPLMDataRefInfo_t info{};
            info.structSize = sizeof(info);
            XPLMGetDataRefInfo(batch[i], &info);
            if (!info.name) continue;

            const std::string name = Lower(info.name);
            const bool hit = std::any_of(lowerTerms.begin(), lowerTerms.end(),
                [&](const std::string& t) { return name.find(t) != std::string::npos; });
            if (!hit) continue;

            char owner[256] = "X-Plane";
            if (info.owner > XPLM_PLUGIN_XPLANE)
                XPLMGetPluginInfo(info.owner, owner, nullptr, nullptr, nullptr);

            double value = 0.0;
            const bool readable = ReadAsDouble(batch[i], value);
            LogMsg("scan: %-70s %-10s %s  owner=%s  value=%s%.4f",
                   info.name, TypeName(info.type).c_str(),
                   info.writable ? "rw" : "ro", owner,
                   readable ? "" : "n/a ", readable ? value : 0.0);
            ++matches;
        }
    }
    LogMsg("scan: done, %d match(es). Private art controls may not be enumerable; "
           "test those by name with 'probe =' lines in RealRain.ini.", matches);
    return matches;
}
