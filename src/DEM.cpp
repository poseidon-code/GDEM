#include <cmath>
#include <cstdint>
#include <string>

#include <gdal/gdal_priv.h>

#include "Type.hpp"
#include "DEM.hpp"



DEM::DEM(const std::string& filepath) {
    GDALRegister_GTiff();

    this->dataset = (GDALDataset *) GDALOpen(filepath.c_str(), GA_ReadOnly);
    if (this->dataset == nullptr) throw std::runtime_error("\nfailed to read DEM file");

    this->type = Type(dataset);

    this->data = this->dataset->GetRasterBand(1);
}


DEM::~DEM() {
    if (this->dataset != nullptr) GDALClose(this->dataset);
}


bool DEM::check_coordinates_bounds(double latitude, double longitude) {
    if (
        latitude >= this->type.y_min
        && latitude < this->type.y_max
        && longitude >= this->type.x_min
        && longitude < this->type.x_max
    ) {
        return true;
    } else {
        return false;
    }
}


DEM::Index DEM::index(double latitude, double longitude) {
    if (check_coordinates_bounds(latitude, longitude)) {
        return {
            (latitude - this->type.y_max) / this->type.y_resolution,
            (longitude - this->type.x_min) / this->type.x_resolution
        };
    } else {
        return {
            static_cast<double>(this->type.nodata),
            static_cast<double>(this->type.nodata)
        };
    }
}


int16_t DEM::altitude(double latitude, double longitude) {
    Index rc = index(latitude, longitude);

    if (rc.row == this->type.nodata || rc.column == this->type.nodata) {
        return this->type.nodata;
    }

    uint64_t r = static_cast<uint64_t>(std::round(rc.row));
    uint64_t c = static_cast<uint64_t>(std::round(rc.column));

    int16_t altitude;
    if (this->data->RasterIO(GF_Read, c, r, 1, 1, &altitude, 1, 1, this->type.data_type, 0, 0) != CE_None) {
        return this->type.nodata;
    } else {
        return altitude;
    }
}


double DEM::interpolated_altitude(double latitude, double longitude) {
    Index rc = index(latitude, longitude);

    if (rc.row == this->type.nodata || rc.column == this->type.nodata) {
        return this->type.nodata;
    }

    uint64_t r = static_cast<uint64_t>(rc.row);
    uint64_t c = static_cast<uint64_t>(rc.column);

    double del_latitude = std::min(rc.row, static_cast<double>(this->type.rows-1)) - r;
    double del_longitude = std::min(rc.column, static_cast<double>(this->type.columns-1)) - c;

    int16_t m, n, o, p;
    if (
        this->data->RasterIO(GF_Read,       c,   r,     1, 1, &m, 1, 1, this->type.data_type, 0, 0) != CE_None
        || this->data->RasterIO(GF_Read,    c+1, r,     1, 1, &n, 1, 1, this->type.data_type, 0, 0) != CE_None
        || this->data->RasterIO(GF_Read,    c,   r+1,   1, 1, &o, 1, 1, this->type.data_type, 0, 0) != CE_None
        || this->data->RasterIO(GF_Read,    c+1, r+1,   1, 1, &p, 1, 1, this->type.data_type, 0, 0) != CE_None
    ) {
        return this->type.nodata;
    } else {
        double altitude =   (1-del_latitude) *  (1-del_longitude) * m +
                            del_longitude *     (1-del_latitude) *  n +
                            (1-del_longitude) * del_latitude *      o +
                            del_latitude *      del_longitude *     p;

        return altitude;
    }
}
