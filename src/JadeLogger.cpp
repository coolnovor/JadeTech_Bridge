#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <functional>
#include <string>

#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include "../include/JadeLogger.h"

namespace JadeLogger {

    // 内部通用发送函数
    static void SendToChat(uint32_t color, const std::wstring& message, bool transient) {
        // 构建富文本：<c=#颜色>内容</c>
        // 注意：GW 引擎的十六进制颜色是 RRGGBB 格式，不带 Alpha
        wchar_t colorBuf[16];
        swprintf_s(colorBuf, L"<c=#%06X>", color);
        
        std::wstring formattedMsg = colorBuf;
        formattedMsg += message;
        formattedMsg += L"</c>";

        // 复制一份数据，防止异步执行时原数据被销毁
        std::wstring msgCopy = formattedMsg;
        
        GW::GameThread::Enqueue([msgCopy, transient]() {
            // WriteChat 的参数：频道, 消息, 发送者, 是否临时消息
            GW::Chat::WriteChat(JADE_CHAN, msgCopy.c_str(), JADE_SENDER, transient);
        });
    }

    void Init() {
        // 设置这个频道的默认颜色（Sender 的颜色设为稍微灰一点，方便区分）
        GW::Chat::SetSenderColor(JADE_CHAN, 0xFFAAAAAA); 
        // 设置默认消息颜色（会被我们的 <c> 标签覆盖，但作为保底）
        GW::Chat::SetMessageColor(JADE_CHAN, 0xFFFFFFFF); 
    }

    void Info(const std::wstring& message, bool transient) {
        SendToChat(JADE_INFO_COL, message, transient);
    }

    void Warning(const std::wstring& message, bool transient) {
        SendToChat(JADE_WARNING_COL, message, transient);
    }

    void Error(const std::wstring& message, bool transient) {
        SendToChat(JADE_ERROR_COL, message, transient);
    }
}
