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


void Utility::Reproject(GDALDataset* source_dataset, std::string destination_filepath, int16_t nodata_value) {
    GDALRegister_GTiff();

    // get source projection systerm
    const OGRSpatialReference *source_srs = source_dataset->GetSpatialRef();

    // get target projection system to WGS84
    OGRSpatialReference target_srs;
    target_srs.importFromEPSG(4326);

    // set target coordinate transformations
    OGRCoordinateTransformation *srs_transformations = OGRCreateCoordinateTransformation(source_srs, &target_srs);
    if (srs_transformations == nullptr) {
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
        throw std::runtime_error("failed to create target dataset");
    }

    // get source's geo-transformations
    double geotransform[6];
    if (source_dataset->GetGeoTransform(geotransform) != CE_None) {
        OGRCoordinateTransformation::DestroyCT(srs_transformations);
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
        GDALClose(output_dataset);
        throw std::runtime_error("unable to read raster data");
    }

    // write data to output
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < columns; x++) {
            int16_t value = buffer[y * columns + x];
            
            if (value == source_band->GetNoDataValue()) {
                value = nodata_value;
            }
            
            if (output_band->RasterIO(GF_Write, x, y, 1, 1, &value, 1, 1, datatype, 0, 0) != CE_None) {
                delete[] buffer;
                OGRCoordinateTransformation::DestroyCT(srs_transformations);
                GDALClose(output_dataset);
                throw std::runtime_error("unable to write raster data");
            }
        }
    }

    // cleanup
    delete[] buffer;
    OGRCoordinateTransformation::DestroyCT(srs_transformations);
    GDALClose(output_dataset);
}


void Utility::Merge(std::vector<std::string> source_filepaths, std::string destination_filepath, int16_t nodata_value) {
    GDALRegister_GTiff();

    if (source_filepaths.empty()) {
        throw std::runtime_error("no input file paths provided");
    }

    // open datasets
    std::vector<GDALDataset*> datasets;
    for (std::string path : source_filepaths)
        datasets.push_back(static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly)));

    // find the bounding box of the merged output
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    double cellsize_x = 0.0;
    double cellsize_y = 0.0;

    for (GDALDataset *dataset : datasets) {
        double geotransform[6];
        if (dataset->GetGeoTransform(geotransform) != CE_None) {
            for (GDALDataset *d : datasets) GDALClose(d);
            throw std::runtime_error("failed to get dataset transformations");
        }

        min_x = std::min(min_x, geotransform[0]);
        min_y = std::min(min_y, geotransform[3] + dataset->GetRasterYSize() * geotransform[5]);
        max_x = std::max(max_x, geotransform[0] + dataset->GetRasterXSize() * geotransform[1]);
        max_y = std::max(max_y, geotransform[3]);
        cellsize_x = std::max(cellsize_x, std::abs(geotransform[1]));
        cellsize_y = std::max(cellsize_y, std::abs(geotransform[5]));
    }

    // create output file
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *output_dataset = driver->Create(
        destination_filepath.c_str(),
        static_cast<int>((max_x - min_x) / cellsize_x),
        static_cast<int>((max_y - min_y) / cellsize_y),
        1,
        GDT_Int16,
        nullptr
    );
    output_dataset->GetRasterBand(1)->SetNoDataValue(nodata_value);

    double output_geotransform[6] = {min_x, cellsize_x, 0, max_y, 0, -cellsize_y};
    output_dataset->SetGeoTransform(output_geotransform);
    output_dataset->SetProjection(datasets[0]->GetProjectionRef());

    int rows = output_dataset->GetRasterYSize();
    int columns = output_dataset->GetRasterXSize();

    // merge
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            std::vector<int16_t> values;

            double x = output_geotransform[0] + j * output_geotransform[1] + i * output_geotransform[2];
            double y = output_geotransform[3] + j * output_geotransform[4] + i * output_geotransform[5];

            for (GDALDataset *dataset : datasets) {
                double geotransform[6];
                if (dataset->GetGeoTransform(geotransform) != CE_None) {
                    for (GDALDataset *d : datasets) GDALClose(d);
                    throw std::runtime_error("failed to get dataset transformations");
                }

                int x_index = static_cast<int>((x - geotransform[0]) / geotransform[1]);
                int y_index = static_cast<int>((y - geotransform[3]) / geotransform[5]);

                if (
                    x_index >= 0 && x_index < dataset->GetRasterXSize()
                    && y_index >= 0 && y_index < dataset->GetRasterYSize()
                ) {
                    int16_t value;

                    if (dataset->GetRasterBand(1)->RasterIO(GF_Read, x_index, y_index, 1, 1, &value, 1, 1, GDT_Int16, 0, 0) != CE_None) {
                        for (GDALDataset *d : datasets) GDALClose(d);
                        GDALClose(output_dataset);
                        throw std::runtime_error("failed to read raster data");
                    }

                    values.push_back(value);
                }
            }

            // add median of values
            int16_t median;
            int values_count = values.size();

            if (values_count == 0) {
                median = 0;
            } else {
                std::sort(values.begin(), values.end());

                if (values_count % 2 == 0) {
                    median = (values[(values_count / 2) - 1] + values[values_count / 2]) / 2;
                } else {
                    median = values[values_count / 2];
                }
            }

            if (output_dataset->GetRasterBand(1)->RasterIO(GF_Write, j, i, 1, 1, &median, 1, 1, GDT_Int16, 0, 0) != CE_None) {
                for (GDALDataset *d : datasets) GDALClose(d);
                GDALClose(output_dataset);
                throw std::runtime_error("failed to write raster data");
            }
        }
    }

    for (GDALDataset *dataset : datasets) GDALClose(dataset);
    GDALClose(output_dataset);
}
