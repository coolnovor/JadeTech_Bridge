#pragma once
#include <string>
#include <GWCA/Managers/ChatMgr.h>

namespace JadeLogger {
    // 初始化：传入当前 DLL 的 HMODULE，用于定位日志文件
    void Init(HMODULE hModule);

    // 输出信息（白色）
    void Info(const std::wstring& message, bool transient = false);
    // 输出警告（黄色）
    void Warning(const std::wstring& message, bool transient = true);
    // 输出错误（红色）
    void Error(const std::wstring& message, bool transient = true);

    // 纯本地文件日志输出（不干扰游戏聊天框）
    void LogFile(const std::string& tag, const std::string& message);
    void LogFileW(const std::string& tag, const std::wstring& message);
}
