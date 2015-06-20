cmake_minimum_required(VERSION 3.2)
project(OculusWithLeap)

list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)

find_package(OculusVR REQUIRED)
include_directories(${OVR_INCLUDE_DIRS})
include_directories("/Users/yan/data/sdks/leap_sdk/LeapSDK/include")
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
find_package(Leap REQUIRED)
include_directories(${LEAP_INCLUDE_DIRS})
add_executable(oculus_with_leap main.cc field_line.cc Quaternion.cc hand_input_listener.cc pen_line.cc oculus.cc)
target_link_libraries(oculus_with_leap ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${LEAP_LIBRARIES} ${OVR_LIBRARY})
