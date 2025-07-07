cmake_minimum_required(VERSION 3.16)
project(VJApp)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV REQUIRED opencv4)
find_package(SDL2 REQUIRED)
find_package(X11 REQUIRED)
pkg_check_modules(RTMIDI rtmidi)

if(NOT RTMIDI_FOUND)
    find_path(RTMIDI_INCLUDE_DIR NAMES RtMidi.h)
    find_library(RTMIDI_LIBRARY NAMES rtmidi)
    set(RTMIDI_LIBRARIES ${RTMIDI_LIBRARY})
    set(RTMIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIR})
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(vj-app ${SOURCES})
target_link_libraries(vj-app 
    ${OPENCV_LIBRARIES} 
    ${SDL2_LIBRARIES} 
    ${RTMIDI_LIBRARIES}
    ${X11_LIBRARIES}
    ${X11_Xinerama_LIB}
)
target_include_directories(vj-app PRIVATE 
    ${OPENCV_INCLUDE_DIRS} 
    ${SDL2_INCLUDE_DIRS} 
    ${RTMIDI_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
    src/
)