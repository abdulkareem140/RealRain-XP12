#include "SimData.h"

#include "Log.h"

bool ReadAsDouble(XPLMDataRef ref, double& out)
{
    if (!ref) return false;
    XPLMDataTypeID t = XPLMGetDataRefTypes(ref);
    if (t & xplmType_Double)      out = XPLMGetDatad(ref);
    else if (t & xplmType_Float)  out = XPLMGetDataf(ref);
    else if (t & xplmType_Int)    out = XPLMGetDatai(ref);
    else if (t & xplmType_FloatArray) {
        float f = 0.0f;
        XPLMGetDatavf(ref, &f, 0, 1);
        out = f;
    }
    else if (t & xplmType_IntArray) {
        int i = 0;
        XPLMGetDatavi(ref, &i, 0, 1);
        out = i;
    }
    else return false;
    return true;
}

SimData::SimData()
{
    values_.resize(Count);
    auto def = [this](Id id, const char* label, const char* units, std::vector<const char*> names) {
        values_[id].label = label;
        values_[id].units = units;
        values_[id].candidates = std::move(names);
    };

    // Precipitation. The XP12 name is first; the XP11 name is kept as a fallback.
    def(PrecipRatio,       "Precip on aircraft", "0-1",
        {"sim/weather/aircraft/precipitation_on_aircraft_ratio"});
    def(RainPercentLegacy, "Rain % (legacy)",    "0-1", {"sim/weather/rain_percent"});

    def(TrueAirspeed,      "TAS",          "m/s", {"sim/flightmodel/position/true_airspeed"});
    def(IndicatedAirspeed, "IAS",          "kt",  {"sim/flightmodel/position/indicated_airspeed"});
    def(GroundSpeed,       "Ground speed", "m/s", {"sim/flightmodel/position/groundspeed"});
    def(ElevationMsl,      "Altitude MSL", "m",   {"sim/flightmodel/position/elevation"});
    def(HeightAgl,         "Height AGL",   "m",   {"sim/flightmodel/position/y_agl"});
    def(OutsideAirTemp,    "OAT",          "C",
        {"sim/weather/aircraft/temperature_ambient_deg_c",
         "sim/cockpit2/temperature/outside_air_temp_degc"});

    // Wind at the aircraft, in OpenGL local axes (m/s).
    def(WindX, "Wind X", "m/s", {"sim/weather/aircraft/wind_now_x_msc", "sim/weather/wind_now_x_msc"});
    def(WindY, "Wind Y", "m/s", {"sim/weather/aircraft/wind_now_y_msc", "sim/weather/wind_now_y_msc"});
    def(WindZ, "Wind Z", "m/s", {"sim/weather/aircraft/wind_now_z_msc", "sim/weather/wind_now_z_msc"});

    def(Pitch,   "Pitch",   "deg", {"sim/flightmodel/position/theta"});
    def(Roll,    "Roll",    "deg", {"sim/flightmodel/position/phi"});
    def(Heading, "Heading", "deg", {"sim/flightmodel/position/psi"});

    def(LocalVx, "Vel X", "m/s", {"sim/flightmodel/position/local_vx"});
    def(LocalVy, "Vel Y", "m/s", {"sim/flightmodel/position/local_vy"});
    def(LocalVz, "Vel Z", "m/s", {"sim/flightmodel/position/local_vz"});

    def(LocalX, "Pos X", "m", {"sim/flightmodel/position/local_x"});
    def(LocalY, "Pos Y", "m", {"sim/flightmodel/position/local_y"});
    def(LocalZ, "Pos Z", "m", {"sim/flightmodel/position/local_z"});

    // Pilot eye point relative to the CG, aircraft coordinates.
    def(EyeX, "Pilot eye X", "m", {"sim/aircraft/view/acf_peX"});
    def(EyeY, "Pilot eye Y", "m", {"sim/aircraft/view/acf_peY"});
    def(EyeZ, "Pilot eye Z", "m", {"sim/aircraft/view/acf_peZ"});

    def(ViewExternal, "External view", "bool", {"sim/graphics/view/view_is_external"});
}

void SimData::Resolve()
{
    for (SimValue& v : values_) {
        v.ref = nullptr;
        v.resolvedName.clear();
        for (const char* name : v.candidates) {
            if (XPLMDataRef r = XPLMFindDataRef(name)) {
                v.ref = r;
                v.resolvedName = name;
                break;
            }
        }
        if (!v.ref) LogMsg("dataref missing for '%s' (tried %zu name(s))", v.label, v.candidates.size());
    }
}

void SimData::Update()
{
    for (SimValue& v : values_) {
        double d = 0.0;
        if (ReadAsDouble(v.ref, d)) v.value = d;
    }
}
