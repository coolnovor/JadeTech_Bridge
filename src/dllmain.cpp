
#include <windows.h>

#include <cwchar>
#include <string>

#include <GWCA/GWCA.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>

#include "JadeTech/Core/JadeLogger.h"
#include "JadeTech/Features/ChatCleaner.h"
#include "JadeTech/IPC/SharedMemory.h"
#include "JadeTech/Runtime/HookState.h"

namespace {

// ============================================================
// DLL 工作线程
// ============================================================

DWORD WINAPI WorkerMain(LPVOID) {
    auto& rt = JadeHook::GetRuntime();

    HMODULE gwca = nullptr;

    bool gwcaInitialized = false;
    bool chatCleanerInitialized = false;

    DWORD result = 0;

    // 初始化日志
    JadeLogger::Init(rt.module);

    JadeLogger::LogFile("SYSTEM", "JadeTech_Bridge 工作线程启动。");

    do {
        // ----------------------------------------------------
        // 检查是否已经收到停止请求
        // ----------------------------------------------------

        if (!rt.running.load()) {
            break;
        }

        // ----------------------------------------------------
        // 获取当前 DLL 所在目录
        // ----------------------------------------------------

        wchar_t path[MAX_PATH]{};

        const DWORD pathLength = GetModuleFileNameW(rt.module, path, MAX_PATH);

        if (pathLength == 0 || pathLength >= MAX_PATH) {
            JadeLogger::LogFile("ERROR", "无法获取 Bridge DLL 路径。");

            result = 1;
            break;
        }

        wchar_t* slash = wcsrchr(path, L'\\');

        if (!slash) {
            JadeLogger::LogFile("ERROR", "无法解析 Bridge DLL 所在目录。");

            result = 2;
            break;
        }

        // 保留目录部分
        *(slash + 1) = L'\0';

        const std::wstring gwcaPath = std::wstring(path) + L"gwca.dll";

        // ----------------------------------------------------
        // 加载 GWCA
        // ----------------------------------------------------

        gwca = LoadLibraryW(gwcaPath.c_str());

        if (!gwca) {
            JadeLogger::LogFile("ERROR", "加载 gwca.dll 失败，错误代码：" + std::to_string(GetLastError()));

            result = 3;
            break;
        }

        // 如果加载过程中收到停止请求，则不再初始化 GWCA
        if (!rt.running.load()) {
            break;
        }

        // ----------------------------------------------------
        // 初始化 GWCA
        // ----------------------------------------------------

        if (!GW::Initialize()) {
            JadeLogger::LogFile("ERROR", "GWCA 初始化失败。");

            result = 4;
            break;
        }

        gwcaInitialized = true;

        JadeLogger::LogFile("SYSTEM", "GWCA 初始化成功。");

        // ----------------------------------------------------
        // 设置 JadeTech 发信者颜色
        // ----------------------------------------------------

        GW::GameThread::Enqueue(
            []() {
                GW::Chat::SetSenderColor(GW::Chat::Channel::CHANNEL_GWCA2,
                                         0xFFCFFF64);
            },
            true);

        // ----------------------------------------------------
        // 初始化独立功能模块
        // ----------------------------------------------------

        if (!rt.running.load()) {
            break;
        }

        JadeLogger::LogFile("SYSTEM", "正在初始化功能模块。");

        // 聊天记录清理模块
        // 初始化失败不影响其他功能。
        chatCleanerInitialized = true;

        if (!JadeTech::Features::ChatCleaner::Initialize()) {
            JadeLogger::LogFile("WARNING",
                                "ChatCleaner 初始化失败，"
                                "已禁用地图切换聊天清理功能。");
        } else {
            JadeLogger::LogFile("SYSTEM", "ChatCleaner 初始化成功。");
        }

        // ----------------------------------------------------
        // 启动共享内存主循环
        // ----------------------------------------------------

        if (rt.running.load()) {
            JadeLogger::LogFile("SYSTEM", "共享内存通信线程启动。");

            result = JadeHook::SharedMemoryThread(nullptr);
        }

    } while (false);

    // ========================================================
    // 统一清理流程
    // ========================================================

    // --------------------------------------------------------
    // 停止功能模块
    // 必须在 GW::Terminate() 之前执行。
    // --------------------------------------------------------

    if (chatCleanerInitialized) {
        JadeTech::Features::ChatCleaner::Terminate();

        JadeLogger::LogFile("SYSTEM", "ChatCleaner 已停止。");
    }

    // --------------------------------------------------------
    // 关闭 GWCA
    // --------------------------------------------------------

    if (gwcaInitialized) {
        GW::DisableHooks();
        GW::Terminate();

        JadeLogger::LogFile("SYSTEM", "GWCA 已关闭。");
    }

    // --------------------------------------------------------
    // 释放 GWCA DLL
    // --------------------------------------------------------

    if (gwca) {
        FreeLibrary(gwca);
        gwca = nullptr;
    }

    // --------------------------------------------------------
    // 更新运行状态
    // --------------------------------------------------------

    rt.running.store(false);

    JadeLogger::LogFile("SYSTEM",
                        "JadeTech_Bridge 工作线程已退出，"
                        "退出代码：" +
                        std::to_string(result));

    return result;
}

}  // namespace

// ============================================================
// DllMain
// 不在 Loader Lock 下初始化 GWCA 或创建工作线程。
// ============================================================

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        JadeHook::GetRuntime().module = instance;

        DisableThreadLibraryCalls(instance);
    }

    return TRUE;
}

// ============================================================
// 初始化导出接口
// 由外部注入器调用。
// ============================================================

extern "C" __declspec(dllexport) BOOL __stdcall JadeTech_Initialize() {
    auto& rt = JadeHook::GetRuntime();

    // 防止重复启动
    bool expected = false;

    if (!rt.running.compare_exchange_strong(expected, true)) {
        return TRUE;
    }

    // 清理上一次工作线程遗留的线程句柄。
    // running 已经为 false，旧线程应当已经结束或正在结束。
    if (rt.worker) {
        const DWORD wait = WaitForSingleObject(rt.worker, INFINITE);

        if (wait != WAIT_OBJECT_0) {
            rt.running.store(false);

            return FALSE;
        }

        CloseHandle(rt.worker);
        rt.worker = nullptr;
    }

    // 创建工作线程
    rt.worker = CreateThread(nullptr, 0, WorkerMain, nullptr, 0, nullptr);

    if (!rt.worker) {
        JadeLogger::LogFile("ERROR", "创建 Bridge 工作线程失败，错误代码：" + std::to_string(GetLastError()));

        rt.running.store(false);

        return FALSE;
    }

    return TRUE;
}

// ============================================================
// 关闭导出接口
// 由外部注入器调用。
// ============================================================

extern "C" __declspec(dllexport) BOOL __stdcall JadeTech_Shutdown(
    DWORD timeoutMs) {
    auto& rt = JadeHook::GetRuntime();

    // 通知共享内存主循环退出
    rt.running.store(false);

    // 等待工作线程完成：
    // 1. 共享内存主循环退出
    // 2. ChatCleaner 清理
    // 3. GWCA 关闭
    // 4. 释放 gwca.dll
    if (rt.worker) {
        const DWORD wait = WaitForSingleObject(rt.worker, timeoutMs);

        if (wait == WAIT_TIMEOUT) {
            return FALSE;
        }

        if (wait != WAIT_OBJECT_0) {
            return FALSE;
        }

        CloseHandle(rt.worker);
        rt.worker = nullptr;
    }

    return TRUE;
}
