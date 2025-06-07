cmake_minimum_required(VERSION 3.16) # 设置CMake最低版本要求

project(EdoyunNet) 

set(CMAKE_CXX_STANDARD 11) # 设置C++标准为C++11

include_directories(${PROJECT_SOURCE_DIR_}/EdoyunNet) # 包含头文件目录

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) # 设置可执行文件输出目录

aux_source_directory(${PROJECT_SOURCE_DIR}/EdoyunNet SRC_LIST) # 获取源文件列表

add_executable(
    EdoyunNet 
    ${SRC_LIST}
) # 添加可执行文件

