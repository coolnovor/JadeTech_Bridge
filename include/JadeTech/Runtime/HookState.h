#pragma once
#include <windows.h>
#include <atomic>

namespace JadeHook {

// 自动进入角色选择界面的状态
enum class EnterState : DWORD {
    Waiting,            // 尚未检测到角色选择界面
    CharacterSelect,    // 已检测到角色选择界面
    RequestSent,        // 已发送进入请求，等待切换迹象
    TransitionStarted,  // 已确认离开角色选择界面，等待地图加载
    Completed,          // 已成功进入游戏
    Failed              // 自动进入失败，停止本次生命周期内的重试
};

struct RuntimeState {
    EnterState enterState = EnterState::Waiting;

    // 自动进入流程控制
    bool autoEnterStarted = false;
    bool autoEnterFinished = false;

    // 点击重试信息
    DWORD enterAttemptCount = 0;
    ULONGLONG enterAttemptTime = 0;
    ULONGLONG transitionStartTime = 0;

    // 点击前的地图状态
    DWORD previousMapId = 0;
    bool previousMapLoaded = false;

    // 当前点击是否被 GWCA 接受
    bool clickAccepted = false;

    // 地图与欢迎提示
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
