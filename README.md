# GDEM
GDEM is a simple header only C++ wrapper library over GDAL for specifically working with GeoTiff DEM data.
It provides basic DEM related functionality like getting the altitude. It also provides some pretty
GeoTiff dataset related functions like getting the metadata of the GeoTiff file.
Utility functions like merge, reproject, clip, resample, etc. are also provided.


## Project Setup (CMake)

> Make sure that `GDAL` libraries are installed on the system _(or is available in `PATH`)_ and that CMake can find it with
> `find_package(GDAL)`.

**Prerequisites**
1. CMake _(version 3.25 or above)_
2. GDAL development libraries
3. C++ compiler with support for C++ 20 standard (or above)


```cmake
# CMakeLists.txt (at the root of your project)

cmake_minimum_required(VERSION 3.25)
project(Test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20) # C++ standard 20 or above
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# including the downloaded GDEM library (<project_root>/external/GDEM)
add_subdirectory(external/GDEM)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIBGDEM_INCLUDE_DIRECTORIES}) # include GDEM headers
target_link_libraries(${PROJECT_NAME} PUBLIC GDEM) # links GDEM
```


## DEM Usage

Basic functionality regarding DEM, like getting the altitude. The dataset is automatically closed on
destruction when DEM object is initialized with DEM file path.

```cpp
#include <cstdint>
#include <filesystem>
#include <string>

#include "GDEM/DEM.hpp"


int main() {
    // initialising DEM
    DEM<int16_t> dem_1(std::filesystem::path("/workspace/data/XYZ.tif"));
    DEM<float, 2> dem_2(std::string("/workspace/data/XYZ.tif"));

    GDALRegister_GTiff();
    GDALDataset* dataset = static_cast<GDALDataset*>(GDALOpen("/workspace/data/XYZ.tif", GA_ReadOnly));
    DEM<int32_t, 3, std::numeric_limits<int32_t>::max()> dem_3(dataset);


    // finding altitude
    float latitude = 14.347826, longitude = 76.567238;
    std::cout << dem_1.altitude(latitude, longitude) << std::endl;
    std::cout << dem_3.interpolated_altitude(latitude, longitude) << std::endl;

    GDALClose(dataset);

    return 0;
}
```


## Utility Usage

1.  **Metadata** \
    **`static void Metadata(GDALDataset* dataset)`** \
    **`static void Metadata(const std::string& file_path)`** \
    **`static void Metadata(const std::filesystem::path& file_path)`**

    Prints the metadata of the input dataset. Usefull when thge underlying data type or the number of raster bands are unknown.
    Takes input dataset as `std::string|std::filesystem::path|GDALDataset*` and prints the relevant DEM metadata.

    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // source dataset file paths
        std::filesystem::path f_1 = "/workspace/data/XYZ.tif";
        std::string f_2 = "/workspace/data/XYZ.tif";

        GDALRegister_GTiff();
        GDALDataset* f_3 = static_cast<GDALDataset*>(GDALOpen("/workspace/data/XYZ.tif", GA_ReadOnly));


        // Metadata
        GDEM::Utility::Metadata(f_1);
        GDEM::Utility::Metadata(f_2);
        GDEM::Utility::Metadata(f_3);


        GDALClose(f_3);
        return 0;
    }
    ```


2.  **Reproject** \
    **`static void Reproject(GDALDataset* source_dataset, const std::string& destination_filepath, int16_t nodata_value)`** \
    **`static void Reproject(const std::string& source_filepath, const std::string& destination_filepath, int16_t nodata_value)`** \
    **`static void Reproject(const std::filesystem::path& source_filepath, const std::string& destination_filepath, int16_t nodata_value)`** \
    **`static void Reproject(const std::string& source_filepath, const std::filesystem::path& destination_filepath, int16_t nodata_value)`** \
    **`static void Reproject(const std::filesystem::path& source_filepath, const std::filesystem::path& destination_filepath, int16_t nodata_value)`**

    Reprojects the dataset from its current Spatial Reference System to a `WGS84/EPSG:4326` Spatial Reference System.
    Takes the input dataset as `std::string|std::filesystem::path|GDALDataset*` with a destination file path of the
    reprojected dataset as `std::string|std::filesystem::path` and the `NODATA` value. \
    It creates a new file with the passed in destination file path as path for output.

    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // source dataset file paths
        std::filesystem::path f_1 = "/workspace/data/XYZ.tif";
        std::string f_2 = "/workspace/data/XYZ.tif";

        GDALRegister_GTiff();
        GDALDataset* f_3 = static_cast<GDALDataset*>(GDALOpen("/workspace/data/XYZ.tif", GA_ReadOnly));

        // destination file paths
        std::filesystem::path d_1 = "/workspace/data/XYZ_reprojected.tif";
        std::string d_2 = "/workspace/data/XYZ_reprojected.tif";


        // Reprojection
        GDEM::Utility::Reproject(f_2, d_1, INT16_MIN);
        GDEM::Utility::Reproject(f_1, d_2, 0);
        GDEM::Utility::Reproject(f_3, d_2, INT16_MAX);


        GDALClose(f_3);
        return 0;
    }
    ```


3.  **Merge** \
    **`static void Merge(const std::vector<GDALDataset*>& source_datasets, const std::string& destination_filepath, int16_t nodata_value)`** \
    **`static void Merge(const std::vector<std::string>& source_filepaths, const std::string& destination_filepath, int16_t nodata_value)`** \
    **`static void Merge(const std::vector<std::filesystem::path>& source_filepaths, const std::filesystem::path& destination_filepath, int16_t nodata_value)`** \
    **`static void Merge(const std::vector<std::filesystem::path>& source_filepaths, const std::string& destination_filepath, int16_t nodata_value)`** \
    **`static void Merge(const std::vector<std::string>& source_filepaths, const std::filesystem::path& destination_filepath, int16_t nodata_value)`**

    Merges multiple datasets together with median values approach.
    Takes an input of a `std::vector<std::string|std::filesystem::path|GDALDataset*>` along with a destination
    file path of the merged dataset as `std::string|std::filesystem::path` and the `NODATA` value. \
    It creates a new file with the passed in destination file path as path for output.

    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // source dataset file paths
        std::vector<std::filesystem::path> f_1 = {"/workspace/data/ABC.tif", "/workspace/data/PQR.tif", "/workspace/data/XYZ.tif"};
        std::vector<std::string> f_2 = {"/workspace/data/ABC.tif", "/workspace/data/PQR.tif", "/workspace/data/XYZ.tif"};

        std::vector<GDALDataset*> f_3
        GDALRegister_GTiff();
        for (const auto& path : f_2) {
            f_3.push_back(static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly)));
        }

        // destination file paths
        std::filesystem::path d_1 = "/workspace/data/ABC_PQR_XYZ_merged.tif";
        std::string d_2 = "/workspace/data/ABC_PQR_XYZ_merged.tif";


        // Merging
        GDEM::Utility::Merge(f_2, d_1, INT16_MIN);
        GDEM::Utility::Merge(f_1, d_2, 0);
        GDEM::Utility::Merge(f_3, d_2, INT16_MAX);


        for (auto& dataset : f_3) {
            GDALClose(dataset);
        }
        return 0;
    }
    ```


4.  **Clip** \
    **`static void Clip(GDALDataset* source_dataset, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`** \
    **`static void Clip(const std::string& source_filepath, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`** \
    **`static void Clip(const std::filesystem::path& source_filepath, const std::string& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`** \
    **`static void Clip(const std::string& source_filepath, const std::filesystem::path& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`** \
    **`static void Clip(const std::filesystem::path& source_filepath, const std::filesystem::path& destination_filepath, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`**

    Clips (crops) a dataset with 4 bounding coordinates. If the bounding coordinates doesn't lies within the
    dataset's bounds, it will just crop upto the maximum bounded area of the input datset.
    Takes an input of source dataset as `std::string|std::filesystem::path|GDALDataset*` along with destination
    file path of the clipped dataset as `std::string|std::filesystem::path` and 4  coordinate bounds of the
    bounded region. \
    It creates a new file with the passed in destination file path as path for output.


    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // source dataset file paths
        std::filesystem::path f_1 = "/workspace/data/XYZ.tif";
        std::string f_2 = "/workspace/data/XYZ.tif";

        GDALRegister_GTiff();
        GDALDataset* f_3 = static_cast<GDALDataset*>(GDALOpen("/workspace/data/XYZ.tif", GA_ReadOnly));

        // destination file paths
        std::filesystem::path d_1 = "/workspace/data/XYZ_clipped.tif";
        std::string d_2 = "/workspace/data/XYZ_clipped.tif";


        // Clipping
        double top_left_x = 75.4, top_left_y = 14.4, bottom_right_x = 75.6, bottom_right_y = 14.2;
        GDEM::Utility::Clip(f_2, d_1, top_left_x, top_left_y, bottom_right_x, bottom_right_y);
        GDEM::Utility::Clip(f_1, d_2, top_left_x, top_left_y, bottom_right_x, bottom_right_y);
        GDEM::Utility::Clip(f_3, d_2, top_left_x, top_left_y, bottom_right_x, bottom_right_y);


        GDALClose(f_3);
        return 0;
    }
    ```


5.  **Resample** \
    **`static void Resample(GDALDataset* source_dataset, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height)`** \
    **`static void Resample(const std::string& source_filepath, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height)`** \
    **`static void Resample(const std::filesystem::path& source_filepath, const std::string& destination_filepath, unsigned int output_width, unsigned int output_height)`** \
    **`static void Resample(const std::string& source_filepath, const std::filesystem::path& destination_filepath, unsigned int output_width, unsigned int output_height)`** \
    **`static void Resample(const std::filesystem::path& source_filepath, const std::filesystem::path& destination_filepath, unsigned int output_width, unsigned int output_height)`**

    Resamples the input dataset to a new dataset with given width and height, preserving the projections
    of the source dataset.
    Takes an input of source dataset as `std::string|std::filesystem::path|GDALDataset*` along with destination
    file path of the clipped dataset as `std::string|std::filesystem::path` and the width & height of the
    output dataset. \
    It creates a new file with the passed in destination file path as path for output.


    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // source dataset file paths
        std::filesystem::path f_1 = "/workspace/data/XYZ.tif";
        std::string f_2 = "/workspace/data/XYZ.tif";

        GDALRegister_GTiff();
        GDALDataset* f_3 = static_cast<GDALDataset*>(GDALOpen("/workspace/data/XYZ.tif", GA_ReadOnly));

        // destination file paths
        std::filesystem::path d_1 = "/workspace/data/XYZ_resampled.tif";
        std::string d_2 = "/workspace/data/XYZ_resampled.tif";


        // Clipping
        unsigned int width = 2000, height = 2000;
        GDEM::Utility::Resample(f_2, d_1, width, height);
        GDEM::Utility::Resample(f_1, d_2, width, height);
        GDEM::Utility::Resample(f_3, d_2, width, height);


        GDALClose(f_3);
        return 0;
    }
    ```


6.  **Coverage** \
    **`static std::vector<std::filesystem::path> Coverage(const std::vector<std::string>& filepaths, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`** \
    **`static std::vector<std::filesystem::path> Coverage(const std::vector<std::filesystem::path>& filepaths, double top_left_x, double top_left_y, double bottom_right_x, double bottom_right_y)`**

    Provides a list of file paths from a list of input file paths which covers a region bounded by 4 coordinates.
    Takes an input of a `std::vector<std::string|std::filesystem::path>` and 4  coordinate bounds of the bounded region
    and returns a `std::vector<std::string|std::filesystem::path>` of file paths covering the bounded region.

    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // searching file paths
        std::vector<std::filesystem::path> f_1 = {"/workspace/data/ABC.tif", "/workspace/data/PQR.tif", "/workspace/data/XYZ.tif"};
        std::vector<std::string> f_2 = {"/workspace/data/ABC.tif", "/workspace/data/PQR.tif", "/workspace/data/XYZ.tif"};

        // Coverage
        double top_left_x = 75.4, top_left_y = 14.4, bottom_right_x = 75.6, bottom_right_y = 14.2;
        std::vector<std::filesystem::path> o_1 = GDEM::Utility::Coverage(f_1, top_left_x, top_left_y, bottom_right_x, bottom_right_y);
        std::vector<std::filesystem::path> o_2 = GDEM::Utility::Coverage(f_2, top_left_x, top_left_y, bottom_right_x, bottom_right_y);


        // list of files covering the bounded region
        for (const auto& path : o_1) {
            std::cout << path.string() << std::endl;
        }

        return 0;
    }
    ```


7.  **CoordinatesAlongPolygon** \
    **`static std::vector<std::pair<float, float>> CoordinatesAlongPolygon(const std::vector<std::pair<float, float>>& polygon_points, float interval_arcseconds = 1.0)`**

    Generates coordinates between 2 or more coordinate points with an interval on passed in arcseconds.
    Takes input of a list of coordinate points (latitude & longitude) as `std::vector<std::pair<float, float>>` with
    intervals (in arcseconds) as `float` (default: 1.0 arcsecond) and returns a list of all the coordinates between those
    passed in points as `std::vector<std::pair<float, float>>` (`first`=latitude & `second`=longitude).

    ```cpp
    #include <filesystem>
    #include <string>

    #include "GDEM/Utility.hpp"

    int main() {
        // points of the polygon
        std::vector<std::pair<float, float>> points = {
            {14.51, 75.31},
            {14.52, 75.32},
            {14.53, 75.33}
        };
        float interval_arcseconds = 0.3;

        // generating points in-between
        std::vector<std::pair<float, float>> generated_points = GDEM::Utility::CoordinatesAlongPolygon(points, interval_arcseconds);
        for (auto point : generated_points) {
            std::cout << point.first << ", " << point.second << std::endl;
        }

        return 0;
    }
    ```


# [GPL v3 License](./LICENSE)

GDEM : C++ wrapper over GDAL for working with DEM data. \
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
