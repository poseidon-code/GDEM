cmake_minimum_required(VERSION 3.28)
project(dem)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

find_package(GDAL REQUIRED)


# libdem
set(LIBDEM_INCLUDE_DIRECTORY 
    "include/"
    ${GDAL_INCLUDE_DIRS}
)
set(LIBDEM_SOURCES 
    "src/DEM.cpp"
    "src/Type.cpp"
    "src/Utility.cpp"
)

# libdem.a / libdem.lib
add_library("${PROJECT_NAME}_static" STATIC ${LIBDEM_SOURCES})
target_include_directories("${PROJECT_NAME}_static" PUBLIC ${LIBDEM_INCLUDE_DIRECTORY})
target_link_libraries("${PROJECT_NAME}_static" PUBLIC GDAL::GDAL)
set_target_properties(
    "${PROJECT_NAME}_static"
    PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    POSITION_INDEPENDENT_CODE ON
)

# libdem.so / libdem.dll
add_library("${PROJECT_NAME}_shared" SHARED ${LIBDEM_SOURCES})
target_include_directories("${PROJECT_NAME}_shared" PUBLIC ${LIBDEM_INCLUDE_DIRECTORY})
target_link_libraries("${PROJECT_NAME}_shared" PUBLIC GDAL::GDAL)
set_target_properties(
    "${PROJECT_NAME}_shared"
    PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    POSITION_INDEPENDENT_CODE ON
)
