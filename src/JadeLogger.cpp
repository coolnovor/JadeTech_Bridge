#include <windows.h>
#include <functional>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include "../include/JadeLogger.h"

namespace JadeLogger {
namespace {
std::wstring g_logPath;
std::mutex g_mutex;
std::string ToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), n, nullptr, nullptr);
    return out;
}
std::string Timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    tm local{}; localtime_s(&local, &tt);
    std::ostringstream s; s << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms;
    return s.str();
}
void SendChat(uint32_t color, const std::wstring& message, bool transient) {
    std::wstring formatted = L"<c=#";
    wchar_t colorText[7]{}; swprintf_s(colorText, L"%06X", color & 0xFFFFFF);
    formatted += colorText; formatted += L">"; formatted += message; formatted += L"</c>";
    GW::GameThread::Enqueue([formatted, transient]() {
        GW::Chat::WriteChat(GW::Chat::Channel::CHANNEL_GWCA2, formatted.c_str(), L"JadeTech", transient);
    }, true);
}
}
void Init(HMODULE module) {
    wchar_t path[MAX_PATH]{};
    if (!module || !GetModuleFileNameW(module, path, MAX_PATH)) return;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) *(slash + 1) = L'\0';
    g_logPath = std::wstring(path) + L"JadeTech.log";
    LogFile("SYSTEM", "Logger initialized.");
}
void LogFile(const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_logPath.empty()) return;
    std::ofstream file(g_logPath, std::ios::binary | std::ios::app);
    if (file) file << '[' << Timestamp() << "] [" << tag << "] " << message << "\r\n";
}
void LogFileW(const std::string& tag, const std::wstring& message) { LogFile(tag, ToUtf8(message)); }
void Info(const std::wstring& message, bool transient) { LogFileW("INFO", message); SendChat(0xFFFFFF, message, transient); }
void Warning(const std::wstring& message, bool transient) { LogFileW("WARN", message); SendChat(0xFFFF44, message, transient); }
void Error(const std::wstring& message, bool transient) { LogFileW("ERROR", message); SendChat(0xFF4444, message, transient); }
}
