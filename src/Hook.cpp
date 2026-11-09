#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

#include "../include/SharedMemory.h"
#include "../include/JadeLogger.h"

DWORD WINAPI SharedMemoryThread(LPVOID lpParam) {
    // 1. 延迟 2 秒，确保 GWCA 完全接管
    Sleep(2000);

    // 2. 发送测试欢迎消息 (现在它会同时写入游戏聊天框和本地文件)
    JadeLogger::Info(L"欢迎使用 JadeTech 翠玉科技！",false);

    // 3. 创建共享内存
    std::wstring memName = GetSharedMemName();
    HANDLE hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SharedMemoryData), memName.c_str());
    if (!hMapFile) {
        JadeLogger::LogFile("ERROR", "创建共享内存失败");
        return 1;
    }

    SharedMemoryData* pBuf = (SharedMemoryData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryData));
    if (!pBuf) { 
        CloseHandle(hMapFile); 
        JadeLogger::LogFile("ERROR", "映射共享内存失败");
        return 1; 
    }

    ZeroMemory(pBuf, sizeof(SharedMemoryData));
    pBuf->Magic = 0x4A616465; 
    JadeLogger::LogFile("SYSTEM", "共享内存已就绪，等待易语言指令...");

    // 4. 监听循环
    while (true) {
        if (pBuf->Magic == 0x4A616465 && pBuf->CommandId == 1) {
            pBuf->Status = 1; 
            std::wstring chatMsg(pBuf->ChatMessage);
            
            JadeLogger::Info(chatMsg); 
            
            pBuf->CommandId = 0; 
            pBuf->Status = 2; 
        }
        Sleep(50);
    }

    return 0;
}
