#cmake -G "MinGW Makefiles" .
cmake_minimum_required(VERSION 2.8)

if(CMAKE_COMPILER_IS_GNUCXX) #Maybe "OR MINGW"
	add_definitions(-std=gnu++0x) #Enable C++0x mode
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package (Boost COMPONENTS thread REQUIRED)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(wxWidgets COMPONENTS core base REQUIRED)
find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR})

include(${wxWidgets_USE_FILE})
include_directories(${Boost_INCLUDE_DIRS})

set(SRCS main.cpp mcl/mcl.cpp mcl/mcl.hpp mcl/mclang.cpp mcl/mclang.hpp mcl/layer.cpp)
add_executable(img_cl ${SRCS})
target_link_libraries(img_cl ${wxWidgets_LIBRARIES} ${OPENCL_LIBRARIES} ${Boost_LIBRARIES})


