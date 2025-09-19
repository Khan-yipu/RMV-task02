cmake_minimum_required(VERSION 3.10)
project(opencv_image_processing)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 添加可执行文件
add_executable(opencv_image_processing src/main.cpp)
add_executable(armor_detector_v2 src/armor_detector_v2.cpp)

# 链接OpenCV库
target_link_libraries(opencv_image_processing ${OpenCV_LIBS})
target_link_libraries(armor_detector_v2 ${OpenCV_LIBS})
