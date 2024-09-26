#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <gdal/gdal_priv.h>

#include "GDEM/Type.hpp"



namespace GDEM {

struct Coordinate {
    float latitude;
    float longitude;

    Coordinate()
        : latitude(0),
        longitude(0)
    {};

    Coordinate(float latitude, float longitude)
        : latitude(latitude),
        longitude(longitude)
    {
        if (latitude > 90 || latitude < -90 || longitude > 180 || longitude < -180) {
            std::string e = "invalid coordinates (" + std::to_string(latitude) +  ":" +  std::to_string(longitude) + ")";
            throw std::runtime_error(e);
        }
    };

    Coordinate(const Coordinate& o) = default;
    Coordinate& operator=(const Coordinate& o) = default;
    Coordinate(Coordinate&& o) noexcept = default;
    Coordinate& operator=(Coordinate&& o) noexcept = default;
    ~Coordinate() = default;

    bool operator<(const Coordinate& o) const {
        if (latitude == o.latitude) {
            return longitude < o.longitude;
        }
        return latitude < o.latitude;
    }

    bool operator==(const Coordinate& o) const {
        return latitude == o.latitude && longitude == o.longitude;
    }
};



struct Bounds {
    Coordinate NW;
    Coordinate NE;
    Coordinate SW;
    Coordinate SE;

    Bounds() = default;

    Bounds(const Coordinate& NW, const Coordinate& NE, const Coordinate& SW, const Coordinate& SE)
        : NW(NW),
        NE(NE),
        SW(SW),
        SE(SE)
    {};

    Bounds(const Bounds& o) = default;
    Bounds& operator=(const Bounds& o) = default;
    Bounds(Bounds&& o) noexcept = default;
    Bounds& operator=(Bounds&& o) noexcept = default;
    ~Bounds() = default;

    bool within(float latitude, float longitude) {
        if (
            latitude >= this->SW.latitude
            && latitude < this->NE.latitude
            && longitude >= this->SW.longitude
            && longitude < this->NE.longitude
        ) {
            return true;
        } else {
            return false;
        }
    }
};


template <
    ValidDataType DataType,
    uint16_t raster_number = 1,
    DataType no_data_fallback = std::numeric_limits<DataType>::min()
>
class DEM {
private:
    struct Index {
        float row;
        float column;
    };

    Index index(float latitude, float longitude) {
        if (this->bounds.within(latitude, longitude)) {
            return {
                (latitude - this->type.y_max) / this->type.y_resolution,
                (longitude - this->type.x_min) / this->type.x_resolution
            };
        } else {
            return {
                static_cast<float>(this->type.nodata),
                static_cast<float>(this->type.nodata)
            };
        }
    }


    GDALDataset *dataset;
    GDALRasterBand *data;
    std::filesystem::path file_path;

    void initialize(GDALDataset* dataset) {
        if (raster_number > this->dataset->GetRasterCount()) {
            GDALClose(this->dataset);
            throw std::runtime_error("invalid raster band " + std::to_string(raster_number));
        }

        this->type = Type<DataType, raster_number, no_data_fallback>(dataset);

        this->data = this->dataset->GetRasterBand(raster_number);

        this->bounds = Bounds(
            Coordinate(this->type.y_max, this->type.x_min),
            Coordinate(this->type.y_max, this->type.x_max),
            Coordinate(this->type.y_min, this->type.x_min),
            Coordinate(this->type.y_min, this->type.x_max)
        );
    }

public:
    Type<DataType, raster_number, no_data_fallback> type;
    Bounds bounds;

    DEM(GDALDataset* dataset)
        : dataset(nullptr),
        data(nullptr)
    {
        this->initialize(dataset);
    }

    DEM(const std::string& file_path)
        : dataset(nullptr),
        data(nullptr),
        file_path(file_path)
    {
        if (!std::filesystem::exists(this->file_path)) {
            throw std::runtime_error("file '" + this->file_path.string() + "' not found");
        }

        GDALRegister_GTiff();

        this->dataset = static_cast<GDALDataset*>(GDALOpen(this->file_path.string().c_str(), GA_ReadOnly));
        if (this->dataset == nullptr) {
            throw std::runtime_error("failed to read DEM file");
        }

        this->initialize(this->dataset);
    }

    DEM(const std::filesystem::path& file_path)
        : dataset(nullptr),
        data(nullptr),
        file_path(file_path)
    {
        if (!std::filesystem::exists(this->file_path)) {
            throw std::runtime_error("file '" + this->file_path.string() + "' not found");
        }

        GDALRegister_GTiff();

        this->dataset = static_cast<GDALDataset*>(GDALOpen(this->file_path.string().c_str(), GA_ReadOnly));
        if (this->dataset == nullptr) {
            throw std::runtime_error("failed to read DEM file");
        }

        this->initialize(this->dataset);
    }

    DEM(const DEM& o)
        : dataset(nullptr),
        data(nullptr),
        type(o.type),
        bounds(o.bounds),
        file_path(o.file_path)
    {
        if (o.dataset) {
            this->dataset = static_cast<GDALDataset*>(GDALOpen(this->file_path.string().c_str(), GA_ReadOnly));
            this->data = this->dataset->GetRasterBand(raster_number);
        }
    }

    DEM& operator=(const DEM& o) {
        if (this != &o) {
            if (this->dataset) {
                GDALClose(this->dataset);
                this->dataset = nullptr;
            }

            this->dataset = nullptr;
            this->data = nullptr;
            this->type = o.type;
            this->bounds = o.bounds;
            this->file_path = o.file_path;

            if (o.dataset) {
                this->dataset = static_cast<GDALDataset*>(GDALOpen(this->file_path.string().c_str(), GA_ReadOnly));
                this->data = this->dataset->GetRasterBand(raster_number);
            }
        }

        return *this;
    }

    DEM(DEM&& o) noexcept
        : dataset(o.dataset),
        data(o.data),
        type(std::move(o.type)),
        bounds(std::move(o.bounds)),
        file_path(std::move(file_path))
    {
        o.dataset = nullptr;
        o.data = nullptr;
    }

    DEM& operator=(DEM&& o) noexcept {
        if (this != &o) {
            if (this->dataset) {
                GDALClose(this->dataset);
                this->dataset = nullptr;
            }

            this->dataset = o.dataset;
            this->data = o.data;
            this->type = std::move(o.type);
            this->bounds = std::move(o.bounds);
            this->file_path = std::move(o.file_path);

            o.dataset = nullptr;
            o.data = nullptr;
        }

        return *this;
    }

    ~DEM() {
        if (this->dataset != nullptr) {
            GDALClose(this->dataset);
            this->dataset = nullptr;
        }
    }

    DataType altitude(float latitude, float longitude) {
        Index rc = index(latitude, longitude);

        if (rc.row == this->type.nodata || rc.column == this->type.nodata) {
            return this->type.nodata;
        }

        size_t r = static_cast<size_t>(std::round(rc.row));
        size_t c = static_cast<size_t>(std::round(rc.column));

        r = r == this->type.nrows ? r - 1 : r;
        c = c == this->type.ncols ? c - 1 : c;

        DataType altitude;
        if (this->data->RasterIO(GF_Read, c, r, 1, 1, &altitude, 1, 1, this->type.data_type, 0, 0) != CE_None) {
            return this->type.nodata;
        } else {
            return altitude;
        }
    }

    float interpolated_altitude(float latitude, float longitude) {
        Index rc = index(latitude, longitude);

        if (rc.row == this->type.nodata || rc.column == this->type.nodata) {
            return this->type.nodata;
        }

        size_t r = static_cast<size_t>(rc.row);
        size_t c = static_cast<size_t>(rc.column);

        float del_latitude = std::min(rc.row, static_cast<float>(this->type.rows-1)) - r;
        float del_longitude = std::min(rc.column, static_cast<float>(this->type.columns-1)) - c;

        size_t next_r = (r == this->type.nrows-1) ? r : r + 1;
        size_t next_c = (c == this->type.ncols-1) ? c : c + 1;

        DataType m, n, o, p;
        if (
            this->data->RasterIO(GF_Read,       c,          r,          1, 1, &m, 1, 1, this->type.data_type, 0, 0) != CE_None
            || this->data->RasterIO(GF_Read,    c+next_c,   r,          1, 1, &n, 1, 1, this->type.data_type, 0, 0) != CE_None
            || this->data->RasterIO(GF_Read,    c,          r+next_r,   1, 1, &o, 1, 1, this->type.data_type, 0, 0) != CE_None
            || this->data->RasterIO(GF_Read,    c+next_c,   r+next_r,   1, 1, &p, 1, 1, this->type.data_type, 0, 0) != CE_None
        ) {
            return this->type.nodata;
        } else {
            float altitude =    (1-del_latitude) *  (1-del_longitude) * m +
                                del_longitude *     (1-del_latitude) *  n +
                                (1-del_longitude) * del_latitude *      o +
                                del_latitude *      del_longitude *     p;

            return altitude;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const DEM& o) {
        os << o.type;
        return os;
    }
};

}
