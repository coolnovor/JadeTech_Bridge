#pragma once
#include <windows.h>
#include <string>

#pragma pack(push, 1)
struct SharedMemoryData {
    DWORD Magic;              // 魔数: 0x4A616465 ("Jade")
    DWORD CommandId;          // 指令ID: 0=空闲, 1=发送聊天消息
    wchar_t ChatMessage[256]; // 要发送的聊天内容 (UTF-16)
    DWORD Status;             // 状态: 0=等待指令, 1=执行中, 2=已完成
};
#pragma pack(pop)

inline std::wstring GetSharedMemName() {
    DWORD pid = GetCurrentProcessId();
    return L"JadeTech_Bridge_SharedMem_" + std::to_wstring(pid);
}