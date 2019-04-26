cmake_minimum_required(VERSION 3.5)

set(ENV{IDF_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/esp-idf)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bluetooth_speaker)
