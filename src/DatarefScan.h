#pragma once

#include <string>
#include <vector>

// Lists every registered dataref whose name contains one of the terms
// (case-insensitive) and writes name / type / writable / owner to Log.txt.
// Uses the XPLM 4.0 dataref enumeration API. Returns the number of matches.
int ScanDatarefs(const std::vector<std::string>& terms);
