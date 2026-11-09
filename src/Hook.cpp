#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <functional>
#include <string>
#include <fstream>

#include "../include/SharedMemory.h"
#include "../include/JadeLogger.h"
#include <GWCA/Managers/MapMgr.h>

// 辅助函数：写日志到文件
void LogToFile(const std::string& msg) {
    std::ofstream log("D:\\JadeTech_Bridge_log.txt", std::ios::app);
    if (log.is_open()) log << msg << std::endl;
}

DWORD WINAPI SharedMemoryThread(LPVOID lpParam) {
    LogToFile("Thread Started.");
    
    // 初始化日志颜色
    JadeLogger::Init();

    // 1. 延迟 2 秒
    Sleep(2000);

    // 2. 发送测试欢迎消息 (用绿色高亮！)
    LogToFile("Attempting to send message via JadeLogger...");
    
    // 我们测试一下不同颜色的输出
    JadeLogger::Info(L"欢迎使用 JadeTech 翠玉科技！");
    // JadeLogger::Warning(L"这是一条警告消息（黄色）！");
    // JadeLogger::Error(L"这是一条错误消息（红色）！", false); // false表示常驻，不随切图消失

    // 3. 创建共享内存
    std::wstring memName = GetSharedMemName();
    HANDLE hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SharedMemoryData), memName.c_str());
    if (!hMapFile) return 1;

    SharedMemoryData* pBuf = (SharedMemoryData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryData));
    if (!pBuf) { CloseHandle(hMapFile); return 1; }

    ZeroMemory(pBuf, sizeof(SharedMemoryData));
    pBuf->Magic = 0x4A616465; 
    LogToFile("Shared Memory Ready.");

    // 4. 监听易语言指令
    while (true) {
        if (pBuf->Magic == 0x4A616465 && pBuf->CommandId == 1) {
            pBuf->Status = 1; 
            std::wstring chatMsg(pBuf->ChatMessage);
            JadeLogger::Info(chatMsg); // 使用 JadeLogger 发送
            pBuf->CommandId = 0; 
            pBuf->Status = 2; 
        }
        Sleep(50);
    }

    return 0;
}
