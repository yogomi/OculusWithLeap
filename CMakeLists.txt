cmake_minimum_required(VERSION 3.2)
project(OculusWithLeap)

list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "/usr/local/share/cmake/Modules")
message("${CMAKE_MODULE_PATH}")

# For OculusRift
find_package(OculusVR REQUIRED)
include_directories(${OVR_INCLUDE_DIRS})
include_directories("/Users/yan/data/sdks/leap_sdk/LeapSDK/include")

# For LeapMotion
find_package(Leap REQUIRED)
include_directories(${LEAP_INCLUDE_DIRS})

find_package(Eigen3 REQUIRED)

# For GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

# For OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# For Boost
FIND_PACKAGE(Boost REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})


add_executable(oculus_with_leap main.cc field_line.cc Quaternion.cc hand_input_listener.cc pen_line.cc oculus.cc)
target_link_libraries(oculus_with_leap ${OPENGL_LIBRARIES} ${EIGEN_LIBS} ${GLFW_LIBRARIES} ${LEAP_LIBRARIES} ${OVR_LIBRARY})
