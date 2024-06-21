#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <gdal/gdal_priv.h>


namespace Utility {
    extern void Reproject(const std::string& source_filepath, const std::string& destination_filepath, int16_t nodata_value);
    extern void Reproject(GDALDataset* source_dataset, const std::string& destination_filepath, int16_t nodata_value);
    extern void Merge(const std::vector<std::string>& source_filepaths, const std::string& destination_filepath, int16_t nodata_value);
    extern void Merge(const std::vector<GDALDataset*>& source_datasets, const std::string& destination_filepath, int16_t nodata_value);
    extern void Clip(const std::string& source_filepath, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y);
    extern void Clip(GDALDataset* source_dataset, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y);
    extern void Resample(const std::string& source_filepath, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height);
    extern void Resample(GDALDataset* source_dataset, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height);
    extern std::vector<std::string> Coverage(const std::vector<std::string>& filepaths, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y);
}
