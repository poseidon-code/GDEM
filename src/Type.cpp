#include <system_error>

#include <gdal/gdal_priv.h>

#include "Type.hpp"


Type::Type(const std::string& filepath) {
    GDALRegister_GTiff();

    GDALDataset *dataset = (GDALDataset *) GDALOpen(filepath.c_str(), GA_ReadOnly);
    if (dataset == NULL) throw std::runtime_error("\nfailed to read DEM file");

    this->projection = dataset->GetProjectionRef();
    this->data_type = dataset->GetRasterBand(1)->GetRasterDataType();
    this->nodata = dataset->GetRasterBand(1)->GetNoDataValue();
    if (this->nodata == 0) this->nodata = INT16_MIN;

    this->rows = dataset->GetRasterYSize();
    this->columns = dataset->GetRasterXSize();

    double transform[6];
    if (dataset->GetGeoTransform(transform) != CE_None) {
        GDALClose(dataset);
        throw std::runtime_error("\nfailed to read dataset transformations");
    }

    this->x_resolution = transform[1];
    this->y_resolution = transform[5];

    this->y_min = transform[3] + this->columns * transform[4] + this->rows * this->y_resolution;
    this->x_min = transform[0];
    this->y_max = transform[3];
    this->x_max = transform[0] + this->columns * this->x_resolution + this->rows * transform[2];

    GDALClose(dataset);
}


Type::Type(GDALDataset* dataset) {
    this->projection = dataset->GetProjectionRef();
    this->data_type = dataset->GetRasterBand(1)->GetRasterDataType();
    this->nodata = dataset->GetRasterBand(1)->GetNoDataValue();
    if (this->nodata == 0) this->nodata = INT16_MIN;
    
    this->rows = dataset->GetRasterYSize();
    this->columns = dataset->GetRasterXSize();

    double transform[6];
    if (dataset->GetGeoTransform(transform) != CE_None) {
        throw std::runtime_error("\nfailed to read dataset transformations");
    }

    this->x_resolution = transform[1];
    this->y_resolution = transform[5];

    this->y_min = transform[3] + this->columns * transform[4] + this->rows * this->y_resolution;
    this->x_min = transform[0];
    this->y_max = transform[3];
    this->x_max = transform[0] + this->columns * this->x_resolution + this->rows * transform[2];
}
