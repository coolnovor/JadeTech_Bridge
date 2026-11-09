#pragma once
#include <string>
#include <GWCA/Managers/ChatMgr.h>

// 定义我们自己的通道和发送者
constexpr auto JADE_CHAN = GW::Chat::Channel::CHANNEL_GWCA2; // 专用系统频道
constexpr auto JADE_SENDER = L"JadeTech";
constexpr auto JADE_INFO_COL = 0xFFFFFF;    // 白色
constexpr auto JADE_WARNING_COL = 0xFFFF44; // 黄色
constexpr auto JADE_ERROR_COL = 0xFF4444;   // 红色

namespace JadeLogger {
    // 初始化（设置默认颜色）
    void Init();

    // 输出信息（白色）
    void Info(const std::wstring& message, bool transient = false);
    
    // 输出警告（黄色）
    void Warning(const std::wstring& message, bool transient = true);

    // 输出错误（红色）
    void Error(const std::wstring& message, bool transient = true);
}
