cmake_minimum_required(VERSION 3.20)
project(getLKGCalibration LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 0)

set(SOURCES 
main.cpp
holoCalibration.cpp
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")
find_package(libusb REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
