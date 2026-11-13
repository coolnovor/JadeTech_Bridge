#pragma once
#include <windows.h>
#include <string>

#pragma pack(push, 1)
struct SharedMemoryData {
    DWORD Magic;              // 魔数: 0x4A616465 ("Jade")
    DWORD CommandId;          // 指令ID: 0=空闲, 1=发送聊天消息
    wchar_t ChatMessage[256]; // 要发送的聊天内容 (UTF-16)
    DWORD Status;             // 状态: 0=等待指令, 1=执行中, 2=已完成

    // === 状态回传 ===
    DWORD ProcessId;          // 当前游戏进程PID
    DWORD IsMapLoaded;        // 0=加载中/未就绪, 1=地图已加载完成
    DWORD MapId;              // 当前地图ID（配合解包可以知道是哪个城）
    DWORD InstanceType;       // 0=Loading, 1=Outpost(城), 2=Explorable(野外)
};
#pragma pack(pop)

inline std::wstring GetSharedMemName() {
    DWORD pid = GetCurrentProcessId();
    return L"JadeTech_Bridge_SharedMem_" + std::to_wstring(pid);
}