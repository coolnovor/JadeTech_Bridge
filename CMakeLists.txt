cmake_minimum_required(VERSION 3.20)
project(JadeTech_Bridge CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定生成 32 位 Windows DLL
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GWCA/include
)

# 链接目录 (指向你拷贝过来的 gwca.lib 所在位置)
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GWCA/lib
)

# 包含所有的源码
set(SOURCES
    src/dllmain.cpp
    src/ChatManager.cpp
    src/Hook.cpp
    src/JadeLogger.cpp    # <--- 日志输出
)

# 生成动态链接库
add_library(JadeTech_Bridge SHARED ${SOURCES})

# 链接 Windows 库和 GWCA
target_link_libraries(JadeTech_Bridge PRIVATE
    gwca
    user32
    kernel32
    psapi
)
