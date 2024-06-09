#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include <gdal/gdal_priv.h>

class Type {
public:
    uint64_t rows;              // no. of DEM values available in row
    uint64_t columns;           // no. of DEM values available in column
    double y_min;               // bottom left latitude
    double x_min;               // bottom left longitude
    double y_max;               // top right latitude
    double x_max;               // top right longitude
    double y_resolution;        // distance (in radians) between every DEM values in row
    double x_resolution;        // distance (in radians) between every DEM values in columns
    int16_t nodata;             // invalid DEM value representation
    std::string projection;     // projection of the dataset
    GDALDataType data_type;     // datatype of the DEM values

    Type() = default;
    Type(const std::string& filepath);
    Type(GDALDataset* dataset);
    ~Type() = default;

    friend std::ostream& operator<<(std::ostream& os, const Type& object) {
        os
            << "Projection : " << object.projection << '\n'
            << "Data Type : " << GDALGetDataTypeName(object.data_type) << '\n'
            << "Rows : " << object.rows << '\n'
            << "Columns : " << object.columns << '\n'
            << "Resolution (latitudinal, longitudinal) : (" << object.y_resolution << ", " << object.x_resolution << ")\n"
            << "Bounded Region {\n"
                << "    North West : (" << object.y_max << ", " << object.x_min << ")\n"
                << "    South East : (" << object.y_min << ", " << object.x_max << ")\n"
            << "}\n"
            << "No Data Value : " << object.nodata;

        return os;
    }
};
