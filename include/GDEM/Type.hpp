#pragma once


#include <concepts>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

#include <gdal/gdal_priv.h>



template <typename DataType>
concept ValidDataType =
    std::is_arithmetic_v<DataType> &&
    !std::is_same_v<DataType, bool> &&
    !std::is_same_v<DataType, char> &&
    !std::is_same_v<DataType, signed char> &&
    !std::is_same_v<DataType, unsigned char> &&
    !std::is_same_v<DataType, char16_t> &&
    !std::is_same_v<DataType, char32_t> &&
    !std::is_same_v<DataType, wchar_t>;



namespace GDEM {

template <
    ValidDataType DataType,
    uint16_t raster_number = 1,
    DataType no_data_fallback = std::numeric_limits<DataType>::min()
>
class Type {
private:
    void initialize(GDALDataset* dataset) {
        GDALRegister_GTiff();

        if (raster_number > dataset->GetRasterCount()) {
            GDALClose(dataset);
            throw std::runtime_error("invalid raster band " + std::to_string(raster_number));
        }

        this->projection = dataset->GetProjectionRef();
        this->nodata = dataset->GetRasterBand(raster_number)->GetNoDataValue();
        if (this->nodata == 0) {
            this->nodata = no_data_fallback;
        }

        this->rows = dataset->GetRasterYSize();
        this->columns = dataset->GetRasterXSize();

        double transform[6];
        if (dataset->GetGeoTransform(transform) != CE_None) {
            GDALClose(dataset);
            throw std::runtime_error("failed to read dataset transformations");
        }

        this->x_resolution = transform[1];
        this->y_resolution = transform[5];

        this->y_min = transform[3] + this->columns * transform[4] + this->rows * this->y_resolution;
        this->x_min = transform[0];
        this->y_max = transform[3];
        this->x_max = transform[0] + this->columns * this->x_resolution + this->rows * transform[2];

        this->data_type = dataset->GetRasterBand(raster_number)->GetRasterDataType();
    }

public:
    size_t rows;                // no. of DEM values available in row
    size_t columns;             // no. of DEM values available in column
    float y_min;                // bottom left latitude
    float x_min;                // bottom left longitude
    float y_max;                // top right latitude
    float x_max;                // top right longitude
    float y_resolution;         // distance (in radians) between every DEM values in row
    float x_resolution;         // distance (in radians) between every DEM values in columns
    DataType nodata;            // invalid DEM value representation
    std::string projection;     // projection of the dataset
    GDALDataType data_type;     // datatype of the DEM values


    Type() = default;


    Type(GDALDataset* dataset) {
        this->initialize(dataset);
    }


    Type(const std::string& file_path) {
        if (!std::filesystem::exists(file_path)) {
            std::string e = "file " + file_path + "not found";
            throw std::runtime_error(e);
        }

        GDALRegister_GTiff();

        GDALDataset *dataset = (GDALDataset*) GDALOpen(file_path.c_str(), GA_ReadOnly);
        if (dataset == nullptr) {
            GDALClose(dataset);
            throw std::runtime_error("failed to read DEM file");
        }

        this->initialize(dataset);

        GDALClose(dataset);
    }


    Type(const std::filesystem::path& file_path) {
        if (!std::filesystem::exists(file_path)) {
            std::string e = "file " + file_path.string() + "not found";
            throw std::runtime_error(e);
        }

        GDALRegister_GTiff();

        GDALDataset *dataset = (GDALDataset*) GDALOpen(file_path.c_str(), GA_ReadOnly);
        if (dataset == nullptr) {
            GDALClose(dataset);
            throw std::runtime_error("failed to read DEM file");
        }

        this->initialize(dataset);

        GDALClose(dataset);
    }

    Type(const Type& o) = default;
    Type& operator=(const Type& o) = default;
    Type(Type&& o) noexcept = default;
    Type& operator=(Type&& o) noexcept = default;
    ~Type() = default;


    friend std::ostream& operator<<(std::ostream& os, const Type& o) {
        os
            << "Projection : " << o.projection << "\n"
            << "Data Type : " << GDALGetDataTypeName(o.data_type) << "\n"
            << "Rows : " << o.rows << "\n"
            << "Columns : " << o.columns << "\n"
            << "Resolution (latitudinal, longitudinal) : (" << o.y_resolution << ", " << o.x_resolution << ")\n"
            << "Bounded Region {\n"
                << "    North West : (" << o.y_max << ", " << o.x_min << ")\n"
                << "    South East : (" << o.y_min << ", " << o.x_max << ")\n"
            << "}\n"
            << "No Data Value : " << o.nodata;

        return os;
    }
};

}
