#include <windows.h>
#include <functional>
#include <string>
#include <algorithm>
#include <GWCA/Managers/GameThreadMgr.h>
#include "../include/JadeHook/HookState.h"
#include "../include/JadeHook/GameState.h"
#include "../include/JadeHook/SharedMemory.h"
#include "../include/JadeLogger.h"

namespace JadeHook {
DWORD WINAPI SharedMemoryThread(LPVOID) {
    const std::wstring name = GetSharedMemName();
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
        0, sizeof(SharedMemoryData), name.c_str());
    if (!mapping) { JadeLogger::LogFile("ERROR", "CreateFileMappingW failed: " + std::to_string(GetLastError())); return 1; }
    const DWORD createError = GetLastError();
    auto* data = static_cast<SharedMemoryData*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryData)));
    if (!data) { JadeLogger::LogFile("ERROR", "MapViewOfFile failed: " + std::to_string(GetLastError())); CloseHandle(mapping); return 1; }

    if (createError != ERROR_ALREADY_EXISTS) {
        ZeroMemory(data, sizeof(*data));
        data->Magic = JADE_SHARED_MAGIC;
        data->ProtocolVersion = JADE_PROTOCOL_VERSION;
        data->ProcessId = GetCurrentProcessId();
    } else if (data->Magic != JADE_SHARED_MAGIC || data->ProtocolVersion != JADE_PROTOCOL_VERSION) {
        JadeLogger::LogFile("ERROR", "Existing shared memory protocol mismatch; refusing to overwrite.");
        UnmapViewOfFile(data); CloseHandle(mapping); return 2;
    }

    JadeLogger::LogFile("SYSTEM", "Shared memory ready: " + std::to_string(GetCurrentProcessId()));
    auto& runtime = GetRuntime();
    RuntimeState& state = runtime.gameState;

    while (runtime.running.load()) {
        // Only one queued game-thread task at a time; avoids unbounded queue growth.
        bool expected = false;
        if (runtime.gameTaskQueued.compare_exchange_strong(expected, true)) {
            GW::GameThread::Enqueue([data, &state, &runtime]() {
                if (runtime.running.load()) UpdateGameState(data, &state);
                runtime.gameTaskQueued.store(false);
            }, true);
        }

        // Minimal single-slot command protocol. The controller must write payload first,
        // then CommandId; it must wait for Status before issuing the next command.
        if (data->Magic == JADE_SHARED_MAGIC && data->CommandId == 1 && data->Status != 1) {
            data->Status = 1;
            data->ChatMessage[255] = L'\0';
            std::wstring message(data->ChatMessage);
            JadeLogger::Info(message, false);
            data->CommandId = 0;
            data->Status = 2;
        }
        Sleep(50);
    }
    // Do not destroy callback state while a queued game-thread task can still reference it.
    while (runtime.gameTaskQueued.load()) Sleep(10);
    UnmapViewOfFile(data);
    CloseHandle(mapping);
    return 0;
}
}
