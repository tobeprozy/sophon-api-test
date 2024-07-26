cmake_minimum_required(VERSION 3.10)

# Set project name
project(rtsp_capture)

# Set build type
# set(CMAKE_BUILD_TYPE "RelMinSizeRelease")


# 警告当error
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
# 未初始化错误
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized -Winit-self")

# set rpath
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#################################################################
# use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -g -lstdc++fs -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -g")

set(SDK "/data/Work/soc0701")
# check_and_replace_path(SDK)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_ASM_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
set(BM_LIBS bmlib bmrt bmcv yuv)
set(OPENCV_LIBS opencv_imgproc opencv_core opencv_highgui opencv_imgcodecs opencv_videoio opencv_calib3d)
set(FFMPEG_LIBS avformat avcodec avutil swscale)

set(SAIL_PATH /opt/sophon/sophon-sail)

add_compile_definitions(USE_BMCV=1)
add_compile_definitions(USE_FFMPEG=1)
add_compile_definitions(USE_OPENCV=1)

include_directories("${SDK}/include/")
include_directories("${SDK}/include/opencv4")
link_directories("${SDK}/lib/")

include_directories("${SAIL_PATH}/include/sail")
set(SAIL_LIBS "${SAIL_PATH}/lib/libsail.so")

set(SRC_FILES test_time_stamp.cpp)
ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES} )

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDK_LIBS}
    ${OPENCV_LIBS}
    ${BM_LIBS}
    ${JPU_LIBS}
    ${SAIL_LIBS}
    ${FFMPEG_LIBS}
)
