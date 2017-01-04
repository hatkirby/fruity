cmake_minimum_required (VERSION 3.1)
project (fruity)

add_subdirectory(vendor/verbly)
add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

include_directories(vendor/verbly/lib vendor/libtwittercpp/src vendor/yaml-cpp/include)
add_executable(fruity fruity.cpp)
set_property(TARGET fruity PROPERTY CXX_STANDARD 11)
set_property(TARGET fruity PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(fruity verbly twitter++ yaml-cpp)
