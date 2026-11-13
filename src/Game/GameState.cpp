#include <windows.h>
#include <cstdint>
#include <functional>
#include <string>

#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/MapMgr.h>

#include "../include/JadeTech/Features/CharacterSelect.h"
#include "../include/JadeTech/Game/GameState.h"
#include "../include/JadeLogger.h"

namespace JadeHook {

namespace {

// 点击后首次检查延迟
constexpr ULONGLONG kFirstCheckDelayMs = 500;

// 单次点击最多观察时间
constexpr ULONGLONG kAttemptTimeoutMs = 2000;

// 确认开始切换后，等待地图加载的最长时间
constexpr ULONGLONG kTransitionTimeoutMs = 30000;

// 最大点击次数
constexpr DWORD kMaxEnterAttempts = 3;

// 欢迎提示延迟
constexpr ULONGLONG kWelcomeDelayMs = 1000;

// 记录一次点击前的地图状态，然后执行点击
void BeginEnterAttempt(RuntimeState* state,
                       DWORD mapId,
                       bool mapLoaded,
                       ULONGLONG now) {
    if (!state || state->autoEnterFinished)
        return;

    state->enterAttemptCount++;
    state->enterAttemptTime = now;

    state->previousMapId = mapId;
    state->previousMapLoaded = mapLoaded;

    state->clickAccepted = ClickPlayButton();

    state->enterState = EnterState::RequestSent;

    JadeLogger::LogFile(
        "SYSTEM", "开始自动进入尝试，第 " +
                      std::to_string(state->enterAttemptCount) +
                      " 次，点击结果：" +
                      std::string(state->clickAccepted ? "成功" : "失败"));
}

// 标记自动进入失败
void FailAutoEnter(RuntimeState* state) {
    if (!state)
        return;

    state->enterState = EnterState::Failed;
    state->autoEnterFinished = true;

    JadeLogger::LogFile("ERROR","自动进入失败，已达到最大重试次数，本次不再自动点击。");
}

// 标记已经开始地图切换
void MarkTransitionStarted(RuntimeState* state, ULONGLONG now) {
    if (!state || state->enterState == EnterState::TransitionStarted)
        return;

    state->enterState = EnterState::TransitionStarted;
    state->transitionStartTime = now;

    JadeLogger::LogFile("SYSTEM","已检测到离开角色选择界面的迹象，停止重复点击，等待地图加载。");
}

// 自动进入状态机
void UpdateAutoEnter(RuntimeState* state,
                     DWORD mapId,
                     bool mapLoaded,
                     ULONGLONG now) {
    if (!state || state->autoEnterFinished)
        return;

    const bool selectorReady = IsCharacterSelectReady();

    // 第一次检测到角色选择界面，启动自动进入流程
    if (!state->autoEnterStarted) {
        if (!selectorReady)
            return;

        state->autoEnterStarted = true;
        state->enterState = EnterState::CharacterSelect;

        JadeLogger::LogFile("SYSTEM","首次检测到角色选择界面，启动自动进入流程。");

        BeginEnterAttempt(state, mapId, mapLoaded, now);
        return;
    }

    // 已经确认开始切换：只等待地图加载，不再点击
    if (state->enterState == EnterState::TransitionStarted) {
        if (mapLoaded) {
            // 地图加载成功
            state->enterState = EnterState::Completed;
            state->autoEnterFinished = true;

            JadeLogger::LogFile("SYSTEM", "地图加载成功，自动进入流程完成。");
            return;
        }

        // 切换已经开始，但地图长时间没有加载
        if (now - state->transitionStartTime >= kTransitionTimeoutMs) {
            state->enterState = EnterState::Failed;
            state->autoEnterFinished = true;

            JadeLogger::LogFile("ERROR", "检测到地图切换，但等待地图加载超时，停止自动进入。");
        }

        return;
    }

    // 已经完成或失败，不再处理
    if (state->enterState == EnterState::Completed ||
        state->enterState == EnterState::Failed) {
        state->autoEnterFinished = true;
        return;
    }

    // 正在等待本次点击的结果
    if (state->enterState == EnterState::RequestSent) {
        const ULONGLONG elapsed = now - state->enterAttemptTime;

        // 至少等待 500ms 后再检查
        if (elapsed < kFirstCheckDelayMs)
            return;

        // 判断是否出现地图切换迹象
        const bool mapIdChanged = (mapId != state->previousMapId);

        // 如果点击前地图已经加载，点击后变成未加载，也视为切换迹象
        const bool mapLoadStateChanged = state->previousMapLoaded && !mapLoaded;

        // 角色选择界面消失，说明游戏正在离开角色选择界面
        const bool selectorClosed = !selectorReady;

        if (mapIdChanged || mapLoadStateChanged || selectorClosed ||
            mapLoaded) {
            MarkTransitionStarted(state, now);

            // 如果地图已经加载完成，可以直接结束
            if (mapLoaded) {
                state->enterState = EnterState::Completed;
                state->autoEnterFinished = true;

                JadeLogger::LogFile("SYSTEM", "地图已加载，自动进入流程完成。");
            }

            return;
        }

        // 尚未出现切换迹象，继续观察到单次超时
        if (elapsed < kAttemptTimeoutMs)
            return;

        // 本次点击没有引起可确认的状态变化
        JadeLogger::LogFile("WARN","点击后未检测到地图切换，准备判断是否重试。");

        if (state->enterAttemptCount >= kMaxEnterAttempts) {
            FailAutoEnter(state);
            return;
        }

        // 只有角色选择界面仍然就绪，才允许再次点击
        if (selectorReady) {
            BeginEnterAttempt(state, mapId, mapLoaded, now);
        } else {
            // 界面已经离开，不能贸然再次点击
            MarkTransitionStarted(state, now);
        }

        return;
    }

    // 其他状态下，如果角色选择界面已经就绪，但自动流程尚未结束，
    // 则启动第一次尝试。
    if (state->enterState == EnterState::CharacterSelect && selectorReady) {
        BeginEnterAttempt(state, mapId, mapLoaded, now);
    }
}

}  // namespace

void UpdateGameState(SharedMemoryData* shared, RuntimeState* state) {
    if (!shared || !state || !GW::GameThread::IsInGameThread())
        return;

    const ULONGLONG now = GetTickCount64();

    const bool loaded = GW::Map::GetIsMapLoaded();
    const DWORD mapId = static_cast<DWORD>(GW::Map::GetMapID());
    const DWORD instance = static_cast<DWORD>(GW::Map::GetInstanceType());

    // 更新共享内存中的游戏状态
    shared->IsMapLoaded = loaded ? 1u : 0u;
    shared->MapId = mapId;
    shared->InstanceType = instance;

    // 定期输出地图状态
    if (now - state->lastStatusLog >= 3000) {
        JadeLogger::LogFile("STATUS",
                            "地图状态：loaded=" + std::to_string(loaded) +
                                "，instance=" + std::to_string(instance) +
                                "，map=" + std::to_string(mapId));

        state->lastStatusLog = now;
    }

    // 地图加载完成
    if (loaded) {
        if (!state->lastMapLoaded) {
            state->mapLoadedTime = now;

            // 只有已经启动过自动进入流程，才将它标记为完成。
            // 如果 Bridge 注入时角色已经在游戏中，之后第一次出现角色选择界面
            // 仍然可以启动自动进入流程。
            if (state->autoEnterStarted) {
                state->enterState = EnterState::Completed;
                state->autoEnterFinished = true;
            }

            JadeLogger::LogFile("SYSTEM", "检测到地图加载完成。");
        }

        // 欢迎提示
        if (!state->welcomed && now - state->mapLoadedTime >= kWelcomeDelayMs) {
            state->welcomed = true;
            JadeLogger::Info(L"欢迎使用 JadeTech 翠玉科技！", true);
            JadeLogger::Info(L"测试聊天内容！", false);
        }
    } else {
        // 从已加载地图转为未加载
        if (state->lastMapLoaded) {
            state->mapLoadedTime = 0;
            state->welcomed = false;
        }

        // 运行自动进入状态机
        UpdateAutoEnter(state, mapId, loaded, now);
    }

    state->lastMapLoaded = loaded;
}

}  // namespace JadeHook
