
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <string>
#include <atomic>

#include "../include/SharedMemory.h"
#include "../include/JadeLogger.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/MemoryMgr.h>
#include <GWCA/GameEntities/Frame.h>

// ============================================================
// JadeTech 自动进入游戏状态
// ============================================================

namespace JadeHook
{
    constexpr DWORD kMagic = 0x4A616465;
    constexpr ULONGLONG kEnterRetryInterval = 2000;
    constexpr ULONGLONG kWelcomeDelay = 1000;

    enum class EnterState
    {
        Waiting,
        CharacterSelect,
        RequestSent,
        Loading,
        Completed
    };

    struct RuntimeState
    {
        EnterState enterState = EnterState::Waiting;

        ULONGLONG lastEnterAttempt = 0;
        ULONGLONG requestTime = 0;
        ULONGLONG mapLoadedTime = 0;
        ULONGLONG lastStatusLog = 0;

        bool lastMapLoaded = false;
        bool welcomed = false;
    };

    // GWToolbox++ ToolboxUtils.cpp 中 CharSelectorContext
    // 的前两个成员布局：
    // +0x00 vtable
    // +0x04 frame_id
    struct CharSelectorContextHeader
    {
        uint32_t vtable;
        uint32_t frame_id;
    };

    // ========================================================
    // 角色选择界面检测
    // 对照 GWToolbox++:
    // GWToolboxdll/Utils/ToolboxUtils.cpp
    // ========================================================

    bool IsCharacterSelectReady()
    {
        uint32_t uiState = 10;

        if (!GW::UI::SendUIMessage(
            GW::UI::UIMessage::kCheckUIState,
            nullptr,
            &uiState))
        {
            return false;
        }

        if (uiState != 2)
            return false;

        GW::UI::Frame* selector =
            GW::UI::GetFrameByLabel(L"Selector");

        if (!selector)
            return false;

        if (!selector->IsCreated() || !selector->IsVisible())
            return false;

        auto* context =
            static_cast<CharSelectorContextHeader*>(
                GW::UI::GetFrameContext(selector));

        if (!context)
            return false;

        if (reinterpret_cast<uintptr_t>(context) <= 0xFFFF)
            return false;

        if (context->frame_id != selector->frame_id)
            return false;

        return true;
    }

    // ========================================================
    // 点击 Play 按钮
    // 必须从游戏线程调用
    // ========================================================

    bool ClickPlayButton()
    {
        GW::UI::Frame* playButton =
            GW::UI::GetFrameByLabel(L"Play");

        if (!playButton)
        {
            JadeLogger::LogFile(
                "WARN",
                "未找到 Play 按钮。"
            );
            return false;
        }

        if (!playButton->IsCreated() ||
            !playButton->IsVisible() ||
            playButton->IsDisabled())
        {
            JadeLogger::LogFile(
                "WARN",
                "Play 按钮当前不可用。"
            );
            return false;
        }

        return GW::UI::ButtonClick(playButton);
    }

    // ========================================================
    // 游戏线程：状态采集和自动进入
    // ========================================================

    void UpdateGameState(
        SharedMemoryData* shared,
        RuntimeState* state)
    {
        if (!shared || !state)
            return;

        const ULONGLONG now = GetTickCount64();

        const bool mapLoaded =
            GW::Map::GetIsMapLoaded();

        const auto instanceType =
            GW::Map::GetInstanceType();

        const auto mapId =
            GW::Map::GetMapID();

        // 同步游戏状态
        shared->IsMapLoaded = mapLoaded ? 1 : 0;
        shared->InstanceType = static_cast<DWORD>(instanceType);
        shared->MapId = static_cast<DWORD>(mapId);

        // 定期输出状态日志
        if (now - state->lastStatusLog >= 3000)
        {
            JadeLogger::LogFile(
                "STATUS",
                "IsLoaded=" + std::to_string(mapLoaded) +
                ", Type=" + std::to_string(
                    static_cast<int>(instanceType)) +
                ", MapId=" + std::to_string(
                    static_cast<int>(mapId))
            );

            state->lastStatusLog = now;
        }

        // ----------------------------------------------------
        // 地图加载状态变化
        // ----------------------------------------------------

        if (mapLoaded)
        {
            if (!state->lastMapLoaded)
            {
                state->mapLoadedTime = now;

                state->enterState = EnterState::Completed;

                JadeLogger::LogFile(
                    "SYSTEM",
                    "检测到地图加载，自动进入游戏成功。"
                );
            }

            if (!state->welcomed &&
                now - state->mapLoadedTime >= kWelcomeDelay)
            {
                JadeLogger::Info(
                    L"欢迎使用 JadeTech 翠玉科技！",
                    false
                );

                state->welcomed = true;

                JadeLogger::LogFile(
                    "SYSTEM",
                    "已发送延迟欢迎语。"
                );
            }
        }
        else
        {
            if (state->lastMapLoaded)
            {
                state->mapLoadedTime = 0;
                state->welcomed = false;

                state->enterState = EnterState::Waiting;

                JadeLogger::LogFile(
                    "SYSTEM",
                    "地图退出加载状态。"
                );
            }

            // ------------------------------------------------
            // 自动进入游戏
            // ------------------------------------------------

            const bool charSelectReady =
                IsCharacterSelectReady();

            if (charSelectReady)
            {
                state->enterState =
                    EnterState::CharacterSelect;

                if (now - state->lastEnterAttempt >=
                    kEnterRetryInterval)
                {
                    state->lastEnterAttempt = now;
                    state->requestTime = now;

                    JadeLogger::LogFile(
                        "SYSTEM",
                        "检测到角色选择界面，准备点击 Play。"
                    );

                    if (ClickPlayButton())
                    {
                        state->enterState =
                            EnterState::RequestSent;

                        JadeLogger::LogFile(
                            "SYSTEM",
                            "Play 按钮点击请求已发送，等待地图加载。"
                        );
                    }
                    else
                    {
                        state->enterState =
                            EnterState::CharacterSelect;

                        JadeLogger::LogFile(
                            "WARN",
                            "Play 按钮点击失败，将在下次检测周期重试。"
                        );
                    }
                }
            }
            else
            {
                if (state->enterState != EnterState::RequestSent)
                {
                    state->enterState = EnterState::Waiting;
                }
            }
        }

        state->lastMapLoaded = mapLoaded;
    }
}

// ============================================================
// 共享内存线程
// ============================================================

DWORD WINAPI SharedMemoryThread(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    Sleep(1000);

    const std::wstring memName = GetSharedMemName();

    HANDLE hMapFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemoryData),
        memName.c_str()
    );

    if (!hMapFile)
    {
        JadeLogger::LogFile(
            "ERROR",
            "CreateFileMappingW 失败。"
        );
        return 1;
    }

    const DWORD mappingError = GetLastError();

    SharedMemoryData* pBuf =
        static_cast<SharedMemoryData*>(
            MapViewOfFile(
                hMapFile,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                sizeof(SharedMemoryData)
            )
        );

    if (!pBuf)
    {
        CloseHandle(hMapFile);

        JadeLogger::LogFile(
            "ERROR",
            "MapViewOfFile 失败。"
        );
        return 1;
    }

    // 注意：如果映射已经存在，不得直接清空。
    if (mappingError != ERROR_ALREADY_EXISTS)
    {
        ZeroMemory(pBuf, sizeof(SharedMemoryData));

        pBuf->Magic = JadeHook::kMagic;
        pBuf->ProcessId = GetCurrentProcessId();
    }
    else if (pBuf->Magic != JadeHook::kMagic)
    {
        JadeLogger::LogFile(
            "ERROR",
            "已有共享内存的 Magic 不匹配。"
        );

        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return 1;
    }

    JadeLogger::LogFile(
        "SYSTEM",
        "共享内存已就绪。"
    );

    JadeHook::RuntimeState runtimeState;

    std::atomic<bool> running{ true };

    // --------------------------------------------------------
    // 主循环
    // --------------------------------------------------------

    while (running.load())
    {
        // 由游戏线程执行 GWCA 状态读取和 UI 操作。
        GW::GameThread::Enqueue(
            [pBuf, &runtimeState]()
            {
                JadeHook::UpdateGameState(
                    pBuf,
                    &runtimeState
                );
            }
        );

        // ----------------------------------------------------
        // 易语言指令处理
        // ----------------------------------------------------

        if (pBuf->Magic == JadeHook::kMagic &&
            pBuf->CommandId == 1)
        {
            pBuf->Status = 1;

            std::wstring chatMsg(pBuf->ChatMessage);

            JadeLogger::Info(chatMsg);

            pBuf->CommandId = 0;
            pBuf->Status = 2;
        }

        Sleep(50);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}
