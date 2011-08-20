#cmake -G "MinGW Makefiles" .
cmake_minimum_required(VERSION 2.8)

if(CMAKE_COMPILER_IS_GNUCXX) #Maybe "OR MINGW"
	add_definitions(-std=gnu++0x) #Enable C++0x mode
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(wxWidgets COMPONENTS core base REQUIRED)
find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR})

include(${wxWidgets_USE_FILE})
set(SRCS main.cpp mcl/mcl.cpp mcl/mcl.hpp mcl/mclang.cpp mcl/mclang.hpp)
add_executable(img_cl ${SRCS})
target_link_libraries(img_cl ${wxWidgets_LIBRARIES} ${OPENCL_LIBRARIES})
