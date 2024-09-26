#pragma once


#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#include <gdal/gdal_priv.h>


namespace GDEM {

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
        std::string e = "file " + file_path + "not found";
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

}