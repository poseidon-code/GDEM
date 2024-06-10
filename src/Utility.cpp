#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

#include <gdal/gdal_priv.h>
#include <gdal/ogr_spatialref.h>
#include <gdal/gdalwarper.h>

#include "Utility.hpp"


void Utility::Reproject(std::string source_filepath, std::string destination_filepath, int16_t nodata_value) {
    GDALRegister_GTiff();

    // open source file
    GDALDataset *source_dataset = (GDALDataset *) GDALOpen(source_filepath.c_str(), GA_ReadOnly);
    if (source_dataset == nullptr) {
        throw std::runtime_error("failed to open source file");
    }

    // get source projection systerm
    const OGRSpatialReference *source_srs = source_dataset->GetSpatialRef();

    // get target projection system to WGS84
    OGRSpatialReference target_srs;
    target_srs.importFromEPSG(4326);

    // set target coordinate transformations
    OGRCoordinateTransformation *srs_transformations = OGRCreateCoordinateTransformation(source_srs, &target_srs);
    if (srs_transformations == nullptr) {
        GDALClose(source_dataset);
        throw std::runtime_error("failed to create coordinate transformations");
    }

    // create target file
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *output_dataset = driver->Create(
        destination_filepath.c_str(),
        source_dataset->GetRasterXSize(),
        source_dataset->GetRasterYSize(),
        source_dataset->GetRasterCount(),
        source_dataset->GetRasterBand(1)->GetRasterDataType(),
        nullptr
    );

    if (output_dataset == nullptr) {
        OGRCoordinateTransformation::DestroyCT(srs_transformations);
        GDALClose(source_dataset);
        throw std::runtime_error("failed to create target dataset");
    }

    // get source's geo-transformations
    double geotransform[6];
    if (source_dataset->GetGeoTransform(geotransform) != CE_None) {
        OGRCoordinateTransformation::DestroyCT(srs_transformations);
        GDALClose(source_dataset);
        GDALClose(output_dataset);
        throw std::runtime_error("failed to get source transformations");
    }
    // set target's geo-transformations
    output_dataset->SetGeoTransform(geotransform);

    // set target projection system
    char *target_srs_wkt = nullptr;
    target_srs.exportToWkt(&target_srs_wkt);
    output_dataset->SetProjection(target_srs_wkt);
    CPLFree(target_srs_wkt);


    // copy over raster data to target dataset
    GDALRasterBand *source_band = source_dataset->GetRasterBand(1);
    GDALRasterBand *output_band = output_dataset->GetRasterBand(1);
    output_band->SetNoDataValue(nodata_value);

    int rows = source_dataset->GetRasterYSize();
    int columns = source_dataset->GetRasterXSize();
    GDALDataType datatype = source_band->GetRasterDataType();

    // create buffer to read row data
    int16_t *buffer = new int16_t[rows * columns];

    // copy data from source
    if (source_band->RasterIO(GF_Read, 0, 0, columns, rows, buffer, columns, rows, datatype, 0, 0) != CE_None) {
        delete[] buffer;
        OGRCoordinateTransformation::DestroyCT(srs_transformations);
        GDALClose(source_dataset);
        GDALClose(output_dataset);
        throw std::runtime_error("unable to read raster data");
    }

    // write data to target
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < columns; x++) {
            int16_t value = buffer[y * columns + x];
            
            if (value == source_band->GetNoDataValue()) {
                value = nodata_value;
            }
            
            if (output_band->RasterIO(GF_Write, x, y, 1, 1, &value, 1, 1, datatype, 0, 0) != CE_None) {
                delete[] buffer;
                OGRCoordinateTransformation::DestroyCT(srs_transformations);
                GDALClose(source_dataset);
                GDALClose(output_dataset);
                throw std::runtime_error("unable to write raster data");
            }
        }
    }

    // cleanup
    delete[] buffer;
    OGRCoordinateTransformation::DestroyCT(srs_transformations);
    GDALClose(source_dataset);
    GDALClose(output_dataset);
}
