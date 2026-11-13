#include "JadeTech/Features/ChatCleaner.h"

#include <atomic>
#include <cstdint>

#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Constants/UIMessages.h>
#include <GWCA/Utilities/Scanner.h>

#include "../../include/JadeLogger.h"

namespace JadeTech::Features::ChatCleaner
{
    namespace
    {
        // ------------------------------------------------------------
        // 游戏内部函数类型
        // ------------------------------------------------------------

        using ClearChatLogFn = void(__cdecl*)();
        using InitChatLogFn = void(__cdecl*)();

        ClearChatLogFn g_clearChatLog = nullptr;
        InitChatLogFn g_initChatLog = nullptr;

        // ------------------------------------------------------------
        // 模块状态
        // ------------------------------------------------------------

        GW::HookEntry g_mapChangeEntry{};

        std::atomic<bool> g_initialized{ false };
        std::atomic<bool> g_active{ false };

        // 防止同一时间重复提交清理任务。
        std::atomic<bool> g_taskQueued{ false };

        // ------------------------------------------------------------
        // 扫描游戏内部函数
        // ------------------------------------------------------------

        bool ResolveFunctions()
        {
            // ClearChatLog：
            // 特征码来源：GWToolbox++ ChatLog.cpp
            // 匹配地址调整：-0x70

            constexpr char clearPattern[] =
                "\x83\xc4\x08\x8d\x77\x0c\xbb\x00\x02\x00\x00";

            constexpr char clearMask[] =
                "xxxxxxxxxxx";

            const uintptr_t clearAddress =
                GW::Scanner::Find(
                    clearPattern,
                    clearMask,
                    -0x70
                );

            if (!clearAddress ||
                !GW::Scanner::IsValidPtr(
                    clearAddress,
                    GW::ScannerSection::Section_TEXT))
            {
                JadeLogger::LogFile(
                    "ERROR",
                    "ChatCleaner: 清空聊天函数扫描失败。"
                );

                return false;
            }

            // InitChatLog：
            // 特征码来源：GWToolbox++ ChatLog.cpp
            // 先定位 CALL 指令，再解析调用目标。

            constexpr char initPattern[] =
                "\x68\x7e\x00\x00\x10\x53";

            constexpr char initMask[] =
                "xxxxxx";

            const uintptr_t callAddress =
                GW::Scanner::Find(
                    initPattern,
                    initMask,
                    -0x05
                );

            if (!callAddress ||
                !GW::Scanner::IsValidPtr(
                    callAddress,
                    GW::ScannerSection::Section_TEXT))
            {
                JadeLogger::LogFile(
                    "ERROR",
                    "ChatCleaner: 初始化聊天函数调用位置扫描失败。"
                );

                return false;
            }

            const uintptr_t initAddress =
                GW::Scanner::FunctionFromNearCall(callAddress);

            if (!initAddress ||
                !GW::Scanner::IsValidPtr(
                    initAddress,
                    GW::ScannerSection::Section_TEXT))
            {
                JadeLogger::LogFile(
                    "ERROR",
                    "ChatCleaner: 初始化聊天函数地址解析失败。"
                );

                return false;
            }

            // 只有两个地址都通过基本检查后才保存。
            g_clearChatLog =
                reinterpret_cast<ClearChatLogFn>(clearAddress);

            g_initChatLog =
                reinterpret_cast<InitChatLogFn>(initAddress);

            JadeLogger::LogFile(
                "SYSTEM",
                "ChatCleaner: 游戏内部函数地址解析成功。"
            );

            return true;
        }

        // ------------------------------------------------------------
        // 实际执行聊天清理
        // ------------------------------------------------------------

        void ClearChatHistory()
        {
            if (!g_active.load())
                return;

            if (!g_clearChatLog || !g_initChatLog)
            {
                JadeLogger::LogFile(
                    "ERROR",
                    "ChatCleaner: 内部函数地址无效，取消清理。"
                );

                return;
            }

            // 第一步：清空原有聊天缓冲区。
            g_clearChatLog();

            // GWToolbox++ 使用相同的验证方式：
            // 清空后聊天缓冲区应该为 nullptr。
            if (GW::Chat::GetChatLog() != nullptr)
            {
                JadeLogger::LogFile(
                    "ERROR",
                    "ChatCleaner: 清空后聊天缓冲区仍然存在，取消初始化。"
                );

                return;
            }

            // 第二步：重新初始化空聊天缓冲区。
            g_initChatLog();

            auto* chatLog = GW::Chat::GetChatLog();

            // 验证新缓冲区及 next 索引。
            if (!chatLog ||
                chatLog->next >= GW::Chat::CHAT_LOG_LENGTH ||
                chatLog->messages[chatLog->next] != nullptr)
            {
                JadeLogger::LogFile(
                    "ERROR",
                    "ChatCleaner: 聊天缓冲区初始化验证失败。"
                );

                return;
            }

            JadeLogger::LogFile(
                "SYSTEM",
                "ChatCleaner: 地图切换后聊天记录已清空。"
            );
        }

        // ------------------------------------------------------------
        // 游戏线程任务
        // ------------------------------------------------------------

        void QueueClearTask()
        {
            if (!g_active.load())
                return;

            bool expected = false;

            // 已经有清理任务等待执行时，不重复提交。
            if (!g_taskQueued.compare_exchange_strong(expected, true))
                return;

            GW::GameThread::Enqueue(
                []()
                {
                    // 模块可能在任务排队期间被关闭。
                    if (g_active.load())
                    {
                        ClearChatHistory();
                    }

                    g_taskQueued.store(false);
                },
                true
            );
        }

        // ------------------------------------------------------------
        // 地图切换回调
        // ------------------------------------------------------------

        void OnMapChange(
            GW::HookStatus*,
            GW::UI::UIMessage,
            void*,
            void*)
        {
            if (!g_active.load())
                return;

            // 不在 UI 回调中直接操作聊天缓冲区。
            // 统一交由下一轮游戏线程任务处理。
            QueueClearTask();
        }
    }

    // ================================================================
    // 对外接口
    // ================================================================

    bool Initialize()
    {
        if (g_initialized.load())
            return true;

        g_active.store(false);
        g_taskQueued.store(false);

        g_clearChatLog = nullptr;
        g_initChatLog = nullptr;

        JadeLogger::LogFile(
            "SYSTEM",
            "ChatCleaner: 开始初始化。"
        );

        // 解析游戏内部函数。
        if (!ResolveFunctions())
        {
            JadeLogger::LogFile(
                "ERROR",
                "ChatCleaner: 初始化失败，聊天清理功能已禁用。"
            );

            return false;
        }

        // 注册地图切换监听。
        g_active.store(true);

        GW::UI::RegisterUIMessageCallback(
            &g_mapChangeEntry,
            GW::UI::UIMessage::kMapChange,
            OnMapChange,
            -0x8000
        );

        g_initialized.store(true);

        JadeLogger::LogFile(
            "SYSTEM",
            "ChatCleaner: 初始化成功，已注册地图切换监听。"
        );

        return true;
    }

    void Terminate()
    {
        if (!g_initialized.load())
            return;

        // 先禁止任何新任务继续执行。
        g_active.store(false);

        // 注销地图切换回调。
        GW::UI::RemoveUIMessageCallback(
            &g_mapChangeEntry
        );

        // 清理函数指针。
        g_clearChatLog = nullptr;
        g_initChatLog = nullptr;

        g_taskQueued.store(false);
        g_initialized.store(false);

        JadeLogger::LogFile(
            "SYSTEM",
            "ChatCleaner: 已停止并注销地图切换监听。"
        );
    }

    bool IsInitialized()
    {
        return g_initialized.load();
    }
}