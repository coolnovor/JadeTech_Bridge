#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include "../include/JadeLogger.h"

namespace JadeLogger {
    constexpr auto JADE_CHAN = GW::Chat::Channel::CHANNEL_GWCA2;
    constexpr auto JADE_SENDER = L"JadeTech";
    constexpr auto JADE_INFO_COL = 0xFFFFFF;    // 白色
    constexpr auto JADE_WARNING_COL = 0xFFFF44; // 黄色
    constexpr auto JADE_ERROR_COL = 0xFF4444;   // 红色

    std::wstring g_logPath;
    std::mutex g_logMutex;

    // 辅助：将 wstring 转为 string (UTF-8)
    std::string WStringToString(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    // 获取当前时间字符串
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        struct tm timeinfo;
        localtime_s(&timeinfo, &in_time_t);

        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    void Init(HMODULE hModule) {
        wchar_t dllPath[MAX_PATH];
        GetModuleFileNameW(hModule, dllPath, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(dllPath, L'\\');
        if (lastSlash) *lastSlash = L'\0';
        g_logPath = std::wstring(dllPath) + L"\\JadeTech.log";

        // 设置消息发送者及聊天框颜色
        GW::Chat::SetSenderColor(JADE_CHAN, 0xFFFFA500);
        GW::Chat::SetMessageColor(JADE_CHAN, 0xFFFFFFFF);

        LogFile("SYSTEM", "JadeLogger 初始化完成，日志路径: " + WStringToString(g_logPath));
    }

    void LogFile(const std::string& tag, const std::string& message) {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::ofstream log(g_logPath, std::ios::app);
        if (log.is_open()) {
            log << "[" << GetTimestamp() << "] [" << tag << "] " << message << std::endl;
        }
    }

    void LogFileW(const std::string& tag, const std::wstring& message) {
        LogFile(tag, WStringToString(message));
    }

    static void SendToChat(uint32_t color, const std::wstring& message, bool transient) {
        wchar_t colorBuf[16];
        swprintf_s(colorBuf, L"<c=#%06X>", color);
        
        std::wstring formattedMsg = colorBuf + message + L"</c>";
        std::wstring msgCopy = formattedMsg;
        
        GW::GameThread::Enqueue([msgCopy, transient]() {
            GW::Chat::WriteChat(JADE_CHAN, msgCopy.c_str(), JADE_SENDER, transient);
        });
    }

    void Info(const std::wstring& message, bool transient) {
        SendToChat(JADE_INFO_COL, message, transient);
        LogFileW("INFO", message);
    }

    void Warning(const std::wstring& message, bool transient) {
        SendToChat(JADE_WARNING_COL, message, transient);
        LogFileW("WARN", message);
    }

    void Error(const std::wstring& message, bool transient) {
        SendToChat(JADE_ERROR_COL, message, transient);
        LogFileW("ERROR", message);
    }
}
