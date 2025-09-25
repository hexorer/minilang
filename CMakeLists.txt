cmake_minimum_required(VERSION 3.22)

project(minis)

set(CMAKE_CXX_STANDARD_REQUIRED 23)

include(FetchContent)

FetchContent_Declare(
    parsi
    GIT_REPOSITORY https://github.com/hexorer/parsi
    GIT_TAG main
)
FetchContent_MakeAvailable(parsi)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/minis)

add_subdirectory(minis/minivm)
