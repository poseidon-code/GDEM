/*
GDEM : C++ wrapper over GDAL for working with DEM data.
Copyright (C) 2024  Pritam Halder

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

Author : Pritam Halder
Email : pritamhalder.portfolio@gmail.com
*/

#pragma once


#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include <gdal/gdal_priv.h>
#include <gdal/gdalwarper.h>
#include <gdal/ogr_spatialref.h>



namespace GDEM {
namespace Utility {

static void Metadata(GDALDataset* dataset) {
    GDALRegister_GTiff();

    std::cout << "Projection : " << dataset->GetProjectionRef() << "\n";
    std::cout << "Rows : " << dataset->GetRasterYSize() << "\n";
    std::cout << "Columns : " << dataset->GetRasterXSize() << "\n";

    double transform[6];
    if (dataset->GetGeoTransform(transform) != CE_None) {
        GDALClose(dataset);
        throw std::runtime_error("failed to read dataset transformations");
    }
    std::cout << "Resolution (latitudinal, longitudinal) : (" << transform[5] << ", " << transform[1] << ")\n";

    std::cout << "Bounded Region {\n"
        << "    North West : (" << transform[3] << ", " << transform[0] << ")\n"
        << "    South East : ("
            << transform[3] + dataset->GetRasterXSize() * transform[4] + dataset->GetRasterYSize() * transform[5] << ", "
            << transform[0] + dataset->GetRasterXSize() * transform[1] + dataset->GetRasterYSize() * transform[2] <<
        ")\n"
    << "}\n";

    for (int i = 1; i <= dataset->GetRasterCount(); ++i) {
        std::cout << "Raster (" << i << ") {\n"
            << "    Data Type : " << GDALGetDataTypeName(dataset->GetRasterBand(i)->GetRasterDataType()) << "\n"
            << "    No Data Value : " << dataset->GetRasterBand(i)->GetNoDataValue() << "\n"
        "}\n";
    }
}


static void Metadata(const std::string& file_path) {
    if (!std::filesystem::exists(file_path)) {
        std::string e = "file (" + file_path + ") not found";
        throw std::runtime_error(e);
    }

    GDALRegister_GTiff();

    GDALDataset *dataset = (GDALDataset*) GDALOpen(file_path.c_str(), GA_ReadOnly);
    if (dataset == nullptr) {
        GDALClose(dataset);
        throw std::runtime_error("failed to read DEM file");
    }

    Metadata(dataset);

    GDALClose(dataset);
}


static void Metadata(const std::filesystem::path& file_path) {
    Metadata(std::filesystem::absolute(file_path).string());
}


void Reproject(GDALDataset* source_dataset, const std::string& destination_filepath, int16_t nodata_value) {
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


void Reproject(const std::string& source_filepath, const std::string& destination_filepath, int16_t nodata_value) {
    if (!std::filesystem::exists(source_filepath)) {
        std::string e = "file (" + source_filepath + ") not found";
        throw std::runtime_error(e);
    }

    GDALRegister_GTiff();

    // open source file
    GDALDataset *source_dataset = (GDALDataset *) GDALOpen(source_filepath.c_str(), GA_ReadOnly);
    if (source_dataset == nullptr) {
        throw std::runtime_error("failed to open source file");
    }

    Reproject(source_dataset, destination_filepath, nodata_value);

    // cleanup
    GDALClose(source_dataset);
}


void Reproject(const std::filesystem::path& source_filepath, const std::string& destination_filepath, int16_t nodata_value) {
    Reproject(source_filepath.string(), destination_filepath, nodata_value);
}


void Reproject(const std::string& source_filepath, const std::filesystem::path& destination_filepath, int16_t nodata_value) {
    Reproject(source_filepath, destination_filepath.string(), nodata_value);
}


void Reproject(const std::filesystem::path& source_filepath, const std::filesystem::path& destination_filepath, int16_t nodata_value) {
    Reproject(source_filepath.string(), destination_filepath.string(), nodata_value);
}



void Merge(const std::vector<GDALDataset*>& source_datasets, const std::string& destination_filepath, int16_t nodata_value) {
    GDALRegister_GTiff();

    if (source_datasets.empty()) {
        throw std::runtime_error("no input datasets provided");
    }

    // find the bounding box of the merged output
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    double cellsize_x = 0.0;
    double cellsize_y = 0.0;

    for (GDALDataset *dataset : source_datasets) {
        double geotransform[6];
        if (dataset->GetGeoTransform(geotransform) != CE_None) {
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
    output_dataset->SetProjection(source_datasets[0]->GetProjectionRef());

    int rows = output_dataset->GetRasterYSize();
    int columns = output_dataset->GetRasterXSize();

    // merge
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            std::vector<int16_t> values;

            double x = output_geotransform[0] + j * output_geotransform[1] + i * output_geotransform[2];
            double y = output_geotransform[3] + j * output_geotransform[4] + i * output_geotransform[5];

            for (GDALDataset *dataset : source_datasets) {
                double geotransform[6];
                if (dataset->GetGeoTransform(geotransform) != CE_None) {
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
                        GDALClose(output_dataset);
                        throw std::runtime_error("failed to read raster data");
                    }

                    values.push_back(value);
                }
            }

            // add median of values
            int16_t median;
            if (values.size() == 0) {
                median = source_datasets[0]->GetRasterBand(1)->GetNoDataValue();
            } else {
                std::sort(values.begin(), values.end());
                median = values[values.size() / 2];
            }

            if (output_dataset->GetRasterBand(1)->RasterIO(GF_Write, j, i, 1, 1, &median, 1, 1, GDT_Int16, 0, 0) != CE_None) {
                GDALClose(output_dataset);
                throw std::runtime_error("failed to write raster data");
            }
        }
    }

    // cleanup
    GDALClose(output_dataset);
}


void Merge(const std::vector<std::string>& source_filepaths, const std::string& destination_filepath, int16_t nodata_value) {
    if (source_filepaths.empty()) {
        throw std::runtime_error("no input file paths provided");
    }

    for (const std::string& path : source_filepaths) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("one or more of source files doesn't exists");
        }
    }

    GDALRegister_GTiff();

    // open datasets
    std::vector<GDALDataset*> datasets;
    for (const std::string& path : source_filepaths) {
        datasets.push_back(static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly)));
    }

    Merge(datasets, destination_filepath, nodata_value);

    // cleanup
    for (GDALDataset *dataset : datasets) GDALClose(dataset);
}


void Merge(const std::vector<std::filesystem::path>& source_filepaths, const std::filesystem::path& destination_filepath, int16_t nodata_value) {
    if (source_filepaths.empty()) {
        throw std::runtime_error("no input file paths provided");
    }

    for (const std::filesystem::path& path : source_filepaths) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("one or more of source files doesn't exists");
        }
    }

    std::vector<std::string> source_filepaths_s;
    for (const std::filesystem::path& path : source_filepaths) {
        source_filepaths_s.push_back(path.string());
    }

    Merge(source_filepaths_s, destination_filepath.string(), nodata_value);
}


void Merge(const std::vector<std::filesystem::path>& source_filepaths, const std::string& destination_filepath, int16_t nodata_value) {
    Merge(source_filepaths, std::filesystem::path(destination_filepath), nodata_value);
}


void Merge(const std::vector<std::string>& source_filepaths, const std::filesystem::path& destination_filepath, int16_t nodata_value) {
    Merge(source_filepaths, destination_filepath.string(), nodata_value);
}


void Clip(GDALDataset* source_dataset, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    GDALRegister_GTiff();

    double geotransform[6];
    if (source_dataset->GetGeoTransform(geotransform) != CE_None) {
        throw std::runtime_error("failed to get dataset transformations");
    }

    int source_x_size = source_dataset->GetRasterXSize();
    int source_y_size = source_dataset->GetRasterYSize();

    // calculate pixel coordinates for the top left and bottom right coordinates
    int start_x = static_cast<int>((top_left_x - geotransform[0]) / geotransform[1]);
    int start_y = static_cast<int>((top_left_y - geotransform[3]) / geotransform[5]);
    int end_x = static_cast<int>((bottom_right_x - geotransform[0]) / geotransform[1]);
    int end_y = static_cast<int>((bottom_right_y - geotransform[3]) / geotransform[5]);

    // ensure the pixel coordinates are within the bounds of the source dataset
    start_x = std::max(0, std::min(start_x, source_x_size));
    start_y = std::max(0, std::min(start_y, source_y_size));
    end_x = std::max(0, std::min(end_x, source_x_size));
    end_y = std::max(0, std::min(end_y, source_y_size));

    int output_x_size = end_x - start_x;
    int output_y_size = end_y - start_y;

    if (output_x_size <= 0 || output_y_size <= 0) {
        throw std::runtime_error("invalid clipping coordinates");
    }

    // create output dataset
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *output_dataset = driver->Create(
        destination_filepath.c_str(),
        output_x_size,
        output_y_size,
        1,
        source_dataset->GetRasterBand(1)->GetRasterDataType(),
        nullptr
    );

    if (output_dataset == nullptr) {
        throw std::runtime_error("failed to create output dataset");
    }

    // calculate geotransform for the output dataset
    double output_geotransform[6];
    output_geotransform[0] = geotransform[0] + start_x * geotransform[1];
    output_geotransform[1] = geotransform[1];
    output_geotransform[2] = 0;
    output_geotransform[3] = geotransform[3] + start_y * geotransform[5];
    output_geotransform[4] = 0;
    output_geotransform[5] = geotransform[5];
    output_dataset->SetGeoTransform(output_geotransform);
    output_dataset->SetProjection(source_dataset->GetProjectionRef());

    // read and write data from source to output band
    GDALRasterBand *source_band = source_dataset->GetRasterBand(1);
    GDALRasterBand *output_band = output_dataset->GetRasterBand(1);

    int data_type_size = GDALGetDataTypeSizeBytes(source_band->GetRasterDataType());
    int buffer_size = output_x_size * output_y_size * data_type_size;
    void *buffer = CPLMalloc(buffer_size);

    if (source_band->RasterIO(GF_Read, start_x, start_y, output_x_size, output_y_size, buffer, output_x_size, output_y_size, source_band->GetRasterDataType(), 0, 0) != CE_None) {
        CPLFree(buffer);
        GDALClose(output_dataset);
        throw std::runtime_error("failed to read raster data");
    }

    if (output_band->RasterIO(GF_Write, 0, 0, output_x_size, output_y_size, buffer, output_x_size, output_y_size, output_band->GetRasterDataType(), 0, 0) != CE_None) {
        CPLFree(buffer);
        GDALClose(output_dataset);
        throw std::runtime_error("failed to write raster data");
    }

    // cleanup
    CPLFree(buffer);
    GDALClose(output_dataset);
}


void Clip(const std::string& source_filepath, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    if (!std::filesystem::exists(source_filepath)) {
        std::string e = "file (" + source_filepath + ") not found";
        throw std::runtime_error(e);
    }

    GDALRegister_GTiff();

    // open source file
    GDALDataset *source_dataset = (GDALDataset *) GDALOpen(source_filepath.c_str(), GA_ReadOnly);
    if (source_dataset == nullptr) {
        throw std::runtime_error("failed to open source file");
    }

    Clip(source_dataset, destination_filepath, top_left_x, top_left_y, bottom_right_x, bottom_right_y);

    // cleanup
    GDALClose(source_dataset);
}


void Clip(const std::filesystem::path& source_filepath, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    Clip(source_filepath.string(), destination_filepath, top_left_x, top_left_y, bottom_right_x, bottom_right_y);
}


void Clip(const std::string& source_filepath, const std::filesystem::path& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    Clip(source_filepath, destination_filepath.string(), top_left_x, top_left_y, bottom_right_x, bottom_right_y);
}


void Clip(const std::filesystem::path& source_filepath, const std::filesystem::path& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    Clip(source_filepath.string(), destination_filepath.string(), top_left_x, top_left_y, bottom_right_x, bottom_right_y);
}


void Resample(GDALDataset* source_dataset, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height) {
    GDALRegister_GTiff();

    double geotransform[6];
    if (source_dataset->GetGeoTransform(geotransform) != CE_None) {
        throw std::runtime_error("failed to get dataset transformations");
    }

    // create output dataset
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *output_dataset = driver->Create(
        destination_filepath.c_str(),
        output_width,
        output_height,
        source_dataset->GetRasterCount(),
        source_dataset->GetRasterBand(1)->GetRasterDataType(),
        nullptr
    );

    if (output_dataset == nullptr) {
        throw std::runtime_error("failed to create output dataset");
    }

    // calculate geotransform for the output dataset
    double output_geotransform[6];
    output_geotransform[0] = geotransform[0];
    output_geotransform[1] = (geotransform[1] * source_dataset->GetRasterXSize()) / output_width;
    output_geotransform[2] = 0;
    output_geotransform[3] = geotransform[3];
    output_geotransform[4] = 0;
    output_geotransform[5] = (geotransform[5] * source_dataset->GetRasterYSize()) / output_height;

    output_dataset->SetGeoTransform(output_geotransform);
    output_dataset->SetProjection(source_dataset->GetProjectionRef());

    // resampling
    if (GDALReprojectImage(source_dataset, nullptr, output_dataset, nullptr, GRA_Med, 0, 0, nullptr, nullptr, nullptr) != CE_None) {
        GDALClose(output_dataset);
        throw std::runtime_error("failed to resample dataset");
    }

    // cleanup
    GDALClose(output_dataset);
}


void Resample(const std::string& source_filepath, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height) {
    if (!std::filesystem::exists(source_filepath)) {
        std::string e = "file (" + source_filepath + ") not found";
        throw std::runtime_error(e);
    }

    GDALRegister_GTiff();

    // open source file
    GDALDataset *source_dataset = (GDALDataset*) (GDALOpen(source_filepath.c_str(), GA_ReadOnly));
    if (source_dataset == nullptr) {
        throw std::runtime_error("failed to open source file");
    }

    Resample(source_dataset, destination_filepath, output_width, output_height);

    // cleanup
    GDALClose(source_dataset);
}


void Resample(const std::filesystem::path& source_filepath, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height) {
    Resample(source_filepath.string(), destination_filepath, output_width, output_height);
}


void Resample(const std::string& source_filepath, const std::filesystem::path& destination_filepath, unsigned int output_width, unsigned int output_height) {
    Resample(source_filepath, destination_filepath.string(), output_width, output_height);
}


void Resample(const std::filesystem::path& source_filepath, const std::filesystem::path& destination_filepath, unsigned int output_width, unsigned int output_height) {
    Resample(source_filepath.string(), destination_filepath.string(), output_width, output_height);
}


std::vector<std::filesystem::path> Coverage(const std::vector<std::string>& filepaths, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    GDALRegister_GTiff();
    std::vector<std::filesystem::path> results;

    for (const std::string& filepath : filepaths) {
        GDALDataset *dataset = (GDALDataset*) (GDALOpen(filepath.c_str(), GA_ReadOnly));
        if (dataset == nullptr) {
            continue;
        }

        double geotransform[6];
        if (dataset->GetGeoTransform(geotransform) != CE_None) {
            GDALClose(dataset);
            continue;
        }

        double min_x = geotransform[0];
        double max_x = geotransform[0] + dataset->GetRasterXSize() * geotransform[1];
        double min_y = geotransform[3] + dataset->GetRasterYSize() * geotransform[5];
        double max_y = geotransform[3];

        // check if the dataset falls within the specified region
        if (
            max_x >= top_left_x && min_x <= bottom_right_x
            && max_y >= bottom_right_y && min_y <= top_left_y
        ) {
            results.push_back(std::filesystem::path(filepath));
        }

        GDALClose(dataset);
    }

    return results;
}


std::vector<std::filesystem::path> Coverage(const std::vector<std::filesystem::path>& filepaths, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y) {
    std::vector<std::string> filepaths_s;
    for (std::filesystem::path path : filepaths) {
        filepaths_s.push_back(path.string());
    }

    return Coverage(filepaths_s, top_left_x, top_left_y, bottom_right_x, bottom_right_y);
}


std::vector<std::pair<float, float>> CoordinatesAlongPolygon(const std::vector<std::pair<float, float>>& polygon_points, float interval_arcseconds = 1.0) {
    if (polygon_points.size() < 2) {
        throw std::runtime_error("at least 2 points are required");
    }

    std::vector<std::pair<float, float>> coordinates;

    auto coordinates_along_line = [interval_arcseconds, &coordinates] (float latitude_a, float longitude_a, float latitude_b, float longitude_b) -> void {
        float distance = std::sqrt(std::pow(latitude_b - latitude_a, 2) + std::pow(longitude_b - longitude_a, 2));
        int steps = static_cast<int>(distance / (interval_arcseconds / 3600.0));

        for (int i = 0; i <= steps; ++i) {
            float fraction = static_cast<float>(i) / steps;
            float latitude = latitude_a + fraction * (latitude_b - latitude_a);
            float longitude = longitude_a + fraction * (longitude_b - longitude_a);
            coordinates.push_back({latitude, longitude});
        }
    };

    int n = polygon_points.size();

    for (int i = 0; i < n-1; i++) {
        float latitude_a = polygon_points[i].first;
        float longitude_a = polygon_points[i].second;
        float latitude_b = polygon_points[i + 1].first;
        float longitude_b = polygon_points[i + 1].second;
        coordinates_along_line(latitude_a, longitude_a, latitude_b, longitude_b);
    }

    return coordinates;
}

}
}