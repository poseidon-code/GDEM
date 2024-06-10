#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <gdal/gdal_priv.h>


namespace Utility {
    extern void Reproject(std::string source_filepath, std::string destination_filepath, int16_t nodata_value);
}
