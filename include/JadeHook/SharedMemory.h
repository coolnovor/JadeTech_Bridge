#pragma once

#include <windows.h>
#include "../SharedMemory.h"

namespace JadeHook {

    // 共享内存工作线程入口，由 dllmain.cpp 创建的工作线程调用。
    DWORD WINAPI SharedMemoryThread(LPVOID lpParam);

}
