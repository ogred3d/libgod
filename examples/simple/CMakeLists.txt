project(simple CXX)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(simple simple.cxx)
target_link_libraries(simple god)
