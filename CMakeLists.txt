cmake_minimum_required(VERSION 3.24)
project(JadeTech_Bridge VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "JadeTech_Bridge must be cross-compiled for Windows.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "GWCA is x86-only. Configure with an i686/Win32 toolchain.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GWCA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GWCA")
set(GWCA_IMPLIB "${GWCA_ROOT}/lib/gwca.lib")
set(GWCA_DLL "${GWCA_ROOT}/bin/gwca.dll")
foreach(f "${GWCA_IMPLIB}" "${GWCA_DLL}")
  if(NOT EXISTS "${f}")
    message(FATAL_ERROR "Missing GWCA dependency: ${f}")
  endif()
endforeach()

add_library(JadeTech_Bridge SHARED
  JadeTech_Bridge.def
  src/dllmain.cpp
  src/Runtime/Hook.cpp
  src/Features/CharacterSelect.cpp
  src/Game/GameState.cpp
  src/IPC/SharedMemory.cpp
  src/Game/ChatManager.cpp
  src/Core/JadeLogger.cpp
  src/Features/ChatCleaner.cpp
)

target_include_directories(JadeTech_Bridge PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${GWCA_ROOT}/include"
)
target_compile_definitions(JadeTech_Bridge PRIVATE
  WIN32_LEAN_AND_MEAN NOMINMAX GWCA_IMPORT
  UNICODE _UNICODE
)
target_compile_options(JadeTech_Bridge PRIVATE /W4 /permissive- /EHsc)
target_link_directories(JadeTech_Bridge PRIVATE "${GWCA_ROOT}/lib")
target_link_libraries(JadeTech_Bridge PRIVATE
    gwca
    delayimp
    user32
    kernel32
)
target_link_options(JadeTech_Bridge PRIVATE
    "/DELAYLOAD:gwca.dll"
)

# gwca.dll is loaded explicitly from the Bridge directory by JadeTech_Initialize.
# Do not use /DELAYLOAD: the explicit load gives deterministic dependency errors.
set_target_properties(JadeTech_Bridge PROPERTIES
  OUTPUT_NAME "JadeTech_Bridge"
  PREFIX ""
  SUFFIX ".dll"
)

add_custom_command(TARGET JadeTech_Bridge POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GWCA_DLL}" "$<TARGET_FILE_DIR:JadeTech_Bridge>/gwca.dll"
  COMMENT "Copying gwca.dll beside JadeTech_Bridge.dll"
)