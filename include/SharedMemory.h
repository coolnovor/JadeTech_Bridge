#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

// Keep this layout synchronized with the Easy Language controller.
// ProtocolVersion is appended at the end to preserve existing field offsets.
#pragma pack(push, 4)
struct SharedMemoryData {
    DWORD Magic = 0;
    DWORD CommandId = 0;          // 0 idle, 1 chat message
    wchar_t ChatMessage[256]{};   // UTF-16, NUL terminated
    DWORD Status = 0;             // 0 idle, 1 processing, 2 completed, 3 error
    DWORD ProcessId = 0;
    DWORD IsMapLoaded = 0;
    DWORD MapId = 0;
    DWORD InstanceType = 0;
    DWORD ProtocolVersion = 1;
};
#pragma pack(pop)

inline constexpr DWORD JADE_SHARED_MAGIC = 0x4A616465;
inline constexpr DWORD JADE_PROTOCOL_VERSION = 1;

inline std::wstring GetSharedMemName() {
    return L"Local\\JadeTech_Bridge_SharedMem_" + std::to_wstring(GetCurrentProcessId());
}
