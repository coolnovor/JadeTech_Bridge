#pragma once
#include <windows.h>
#include <atomic>

namespace JadeHook {
enum class EnterState : DWORD { Waiting, CharacterSelect, RequestSent, Completed };
struct RuntimeState {
    EnterState enterState = EnterState::Waiting;
    ULONGLONG lastEnterAttempt = 0;
    ULONGLONG mapLoadedTime = 0;
    ULONGLONG lastStatusLog = 0;
    bool lastMapLoaded = false;
    bool welcomed = false;
};
struct BridgeRuntime {
    std::atomic<bool> running{false};
    std::atomic<bool> gameTaskQueued{false};
    HANDLE worker = nullptr;
    HANDLE stopEvent = nullptr;
    HMODULE module = nullptr;
    RuntimeState gameState{};
};
BridgeRuntime& GetRuntime();
}
