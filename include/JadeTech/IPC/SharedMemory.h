
#pragma once

#include <windows.h>
#include <cstdint>
#include <string>

// ============================================================
// 共享内存通信协议
// 注意：字段顺序、类型、对齐方式必须与易语言端保持一致。
// ============================================================

// ProtocolVersion 放在结构体末尾，避免改变原有字段偏移。
#pragma pack(push, 4)
struct SharedMemoryData {
    DWORD Magic = 0;
    DWORD CommandId = 0;          // 0：空闲，1：聊天消息
    wchar_t ChatMessage[256]{};   // UTF-16，必须以 NULL 结尾
    DWORD Status = 0;              // 0：空闲，1：处理中，2：完成，3：错误
    DWORD ProcessId = 0;
    DWORD IsMapLoaded = 0;
    DWORD MapId = 0;
    DWORD InstanceType = 0;
    DWORD ProtocolVersion = 1;
};
#pragma pack(pop)

// 协议常量
inline constexpr DWORD JADE_SHARED_MAGIC = 0x4A616465;
inline constexpr DWORD JADE_PROTOCOL_VERSION = 1;

// 获取当前游戏进程对应的共享内存名称
inline std::wstring GetSharedMemName() {
    return L"Local\\JadeTech_Bridge_SharedMem_"
        + std::to_wstring(GetCurrentProcessId());
}

// ============================================================
// 共享内存工作线程接口
// ============================================================

namespace JadeHook {

// 共享内存主循环，由 WorkerMain 调用。
DWORD WINAPI SharedMemoryThread(LPVOID lpParam);

}
