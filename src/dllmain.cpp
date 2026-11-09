#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GWCA/GWCA.h> // 引入 GWCA 的总头文件

// 声明外部函数
DWORD WINAPI SharedMemoryThread(LPVOID lpParam);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // 创建新线程执行
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            // 1. 初始化 GWCA 底层环境
            GW::Initialize(); 

            // 2. 启动业务逻辑
            SharedMemoryThread(nullptr);
            return 0;
        }, nullptr, 0, nullptr);
    }
    return TRUE;
}
