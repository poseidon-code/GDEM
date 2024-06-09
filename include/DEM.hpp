#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <gdal/gdal_priv.h>

#include "Type.hpp"


class DEM {
private:
    struct Index {
        double row;
        double column;
    };

    Index index(double latitude, double longitude);

    GDALDataset *dataset;
    GDALRasterBand *data;

public:
    Type type;

    DEM(const std::string& filepath);
    ~DEM();

    bool check_coordinates_bounds(double latitude, double longitude);
    int16_t altitude(double latitude, double longitude);
    double interpolated_altitude(double latitude, double longitude);

    friend std::ostream& operator<<(std::ostream& os, const DEM& object) {
        os << object.type;
        return os;
    }
};
