
cmake_minimum_required(VERSION 3.22.1)

project(HadirEDCCJNI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(EDCC_ROOT "" CACHE PATH "EDCC source directory")
set(EDCC_LIBRARY "" CACHE FILEPATH "Path to libedcc.a")

if(NOT EXISTS "${EDCC_ROOT}/include/edcc/c_api.h")
    message(FATAL_ERROR "EDCC headers were not found: ${EDCC_ROOT}")
endif()

if(NOT EXISTS "${EDCC_LIBRARY}")
    message(FATAL_ERROR "libedcc.a was not found: ${EDCC_LIBRARY}")
endif()

find_package(OpenCV REQUIRED)

add_library(edcc STATIC IMPORTED GLOBAL)

set_target_properties(edcc PROPERTIES
    IMPORTED_LOCATION "${EDCC_LIBRARY}"
)

add_library(hadir_edcc SHARED hadir_edcc_jni.cpp)

target_include_directories(hadir_edcc PRIVATE
    "${EDCC_ROOT}/include"
    "${EDCC_ROOT}/src"
    ${OpenCV_INCLUDE_DIRS}
)

target_compile_options(hadir_edcc PRIVATE
    -fexceptions
    -fopenmp
)

target_link_options(hadir_edcc PRIVATE
    -fopenmp
    -static-openmp
)

target_link_libraries(hadir_edcc PRIVATE
    edcc
    ${OpenCV_LIBS}
    android
    log
    z
    dl
    m
)

set_target_properties(hadir_edcc PROPERTIES
    OUTPUT_NAME "hadir_edcc"
)

message(STATUS "Hadir EDCC JNI configuration completed")
message(STATUS "EDCC root: ${EDCC_ROOT}")
message(STATUS "EDCC library: ${EDCC_LIBRARY}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
