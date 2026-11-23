#include "StreakControl.h"

#include "Log.h"
#include "SimData.h"

namespace {

void WriteDouble(XPLMDataRef ref, double v)
{
    XPLMDataTypeID t = XPLMGetDataRefTypes(ref);
    if (t & xplmType_Double)      XPLMSetDatad(ref, v);
    else if (t & xplmType_Float)  XPLMSetDataf(ref, static_cast<float>(v));
    else if (t & xplmType_Int)    XPLMSetDatai(ref, static_cast<int>(v));
}

} // namespace

void StreakControl::SetOverrides(const std::vector<std::pair<std::string, double>>& list)
{
    if (applied_) Restore();
    entries_.clear();
    for (const auto& [name, value] : list) {
        Entry e;
        e.name = name;
        e.target = value;
        e.ref = XPLMFindDataRef(name.c_str());
        e.writable = e.ref && XPLMCanWriteDataRef(e.ref);
        if (!e.ref)           LogMsg("override: dataref not found: %s", name.c_str());
        else if (!e.writable) LogMsg("override: dataref is read-only: %s", name.c_str());
        entries_.push_back(e);
    }
}

void StreakControl::Apply()
{
    if (applied_) return;
    for (Entry& e : entries_) {
        if (!e.writable) continue;
        ReadAsDouble(e.ref, e.original);
        WriteDouble(e.ref, e.target);
        LogMsg("override: %s %.4f -> %.4f", e.name.c_str(), e.original, e.target);
    }
    applied_ = true;
}

void StreakControl::Restore()
{
    if (!applied_) return;
    for (Entry& e : entries_) {
        if (!e.writable) continue;
        WriteDouble(e.ref, e.original);
        LogMsg("override: restored %s = %.4f", e.name.c_str(), e.original);
    }
    applied_ = false;
}
