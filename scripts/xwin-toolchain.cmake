# 针对 xwin 提供的 MSVC/Windows SDK，使用 clang-cl 交叉编译 x86 目标
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

# 指定编译器
set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER       lld-link)
set(CMAKE_AR           llvm-lib)
set(CMAKE_RC_COMPILER  llvm-rc)
set(CMAKE_MT           llvm-mt)

# 验证 XWIN_SDK 环境变量
if(NOT XWIN_SDK)
    set(XWIN_SDK "$ENV{XWIN_SDK}")
endif()
if(NOT XWIN_SDK)
    message(FATAL_ERROR "请先运行 setup-toolchain.sh 并设置 XWIN_SDK 环境变量！")
endif()

# 包含头文件路径 (MSVC CRT + UCRT + UM)
set(XWIN_INCLUDE_FLAGS
    "/imsvc ${XWIN_SDK}/crt/include \
     /imsvc ${XWIN_SDK}/sdk/include/ucrt \
     /imsvc ${XWIN_SDK}/sdk/include/um \
     /imsvc ${XWIN_SDK}/sdk/include/shared")

set(CMAKE_C_FLAGS_INIT   "--target=i686-pc-windows-msvc ${XWIN_INCLUDE_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "--target=i686-pc-windows-msvc ${XWIN_INCLUDE_FLAGS}")
set(CMAKE_RC_FLAGS_INIT  "-I ${XWIN_SDK}/sdk/include/um -I ${XWIN_SDK}/sdk/include/shared -I ${XWIN_SDK}/crt/include")

# 链接库路径
set(XWIN_LIBRARY_FLAGS
    "/libpath:${XWIN_SDK}/crt/lib/x86 \
     /libpath:${XWIN_SDK}/sdk/lib/ucrt/x86 \
     /libpath:${XWIN_SDK}/sdk/lib/um/x86")

foreach(TARGET_KIND EXE SHARED MODULE)
    set(CMAKE_${TARGET_KIND}_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
endforeach()

# 查找规则设定
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BEFORE)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

# 统一静态链接 CRT（防 DLL 缺失依赖）
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_C_FLAGS_DEBUG     "/MTd /Z7 /Ob0 /Od" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG   "/MTd /Z7 /Ob0 /Od" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE   "/MT /O2 /Oi /Gy /DNDEBUG /Z7" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Oi /Gy /DNDEBUG /Z7" CACHE STRING "")
