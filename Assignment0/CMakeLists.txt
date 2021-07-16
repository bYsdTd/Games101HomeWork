cmake_minimum_required (VERSION 2.8.11)
project (Transformation)

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
# include_directories("../")
message("${Eigen3_DIR}")
message("${EIGEN3_INCLUDE_DIR}")

add_executable (Transformation main.cpp)
