#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <gdal/gdal_priv.h>


namespace Utility {
    extern void Reproject(std::string source_filepath, std::string destination_filepath, int16_t nodata_value);
    extern void Reproject(GDALDataset* source_dataset, std::string destination_filepath, int16_t nodata_value);
    extern void Merge(std::vector<std::string> source_filepaths, std::string destination_filepath, int16_t nodata_value);
    extern void Merge(std::vector<GDALDataset*> source_datasets, std::string destination_filepath, int16_t nodata_value);
    extern void Clip(std::string source_filepath, std::string destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y);
    extern void Clip(GDALDataset* source_dataset, std::string destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y);
    extern void Resample(std::string source_filepath, std::string destination_filepath, unsigned int output_width, unsigned int output_height);
    extern void Resample(GDALDataset* source_dataset, std::string destination_filepath, unsigned int output_width, unsigned int output_height);
}
