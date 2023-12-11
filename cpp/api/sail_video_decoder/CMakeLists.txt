cmake_minimum_required(VERSION 3.10)
project(test)
include(${CMAKE_SOURCE_DIR}/../../cmake/common.cmake)

# SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# message("cur dir: ${CMAKE_CURRENT_SOURCE_DIR}")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
message("cur dir: ${CMAKE_CURRENT_BINARY_DIR}")


SET(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "-O3")
add_definitions(-w)

if (NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH pcie)
endif()

if (${TARGET_ARCH} STREQUAL "pcie")
   find_package(OpenCV REQUIRED)
   include_directories(${OPENCV_INCLUDE_DIRS})

   find_package(libsophon REQUIRED)
   include_directories(${LIBSOPHON_INCLUDE_DIRS})

   set(SAIL_DIR /opt/sophon/sophon-sail/lib/cmake)
   find_package(SAIL REQUIRED)
   include_directories(${SAIL_INCLUDE_DIRS})
   link_directories(${SAIL_LIB_DIRS})

   set(FFMPEG_DIR /opt/sophon/sophon-ffmpeg-latest/lib/cmake)
   find_package(FFMPEG REQUIRED)
   include_directories(${FFMPEG_INCLUDE_DIRS})
   link_directories(${FFMPEG_LIB_DIRS})

   message(STATUS "SAIL_INCLUDE_DIRS: ${SAIL_INCLUDE_DIRS}")
   message(STATUS "SAIL_LIB_DIRS: ${SAIL_LIB_DIRS}")
   message(STATUS "FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
    
   aux_source_directory(. SRC_FILES)
   add_executable(test.pcie ${SRC_FILES})
   target_link_libraries(test.pcie sail)

elseif (${TARGET_ARCH} STREQUAL "soc")
    check_and_replace_path(SDK)
    check_and_replace_path(SAIL_PATH)
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_ASM_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
    set(BM_LIBS bmlib bmrt bmcv yuv)
    set(JPU_LIBS bmjpuapi bmjpulite)
    set(OPENCV_LIBS opencv_imgproc opencv_core opencv_highgui opencv_imgcodecs opencv_videoio)

    include_directories("../../include/")
    
    message("SOC-SDK: ${SDK}")
    include_directories("${SDK}/include/")
    include_directories("${SDK}/include/opencv4/")
    link_directories("${SDK}/lib/")

    message("SAIL_PATH: ${SAIL_PATH}")
    include_directories("${SAIL_PATH}/include/sail")
    set(SAIL_LIBS "${SAIL_PATH}/lib/libsail.so")
    add_compile_definitions(USE_BMCV=1)
    add_compile_definitions(USE_FFMPEG=1)
    add_compile_definitions(USE_OPENCV=1)
    aux_source_directory(. SRC_FILES)
    add_executable(test.soc ${SRC_FILES})
    target_link_libraries(test.soc ${BM_LIBS} ${OPENCV_LIBS} ${JPU_LIBS} ${SAIL_LIBS})
endif ()
