add_library(catch INTERFACE)
target_include_directories(catch INTERFACE catch/single_include)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
