#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

// 引入 GWCA 的总头文件
#include <GWCA/GWCA.h>
// 引入我们重构的日志库
#include "../include/JadeLogger.h"

DWORD WINAPI SharedMemoryThread(LPVOID lpParam);

void LoadGWCADependency(HMODULE hModule) {
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(hModule, dllPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(dllPath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    std::wstring gwcaPath = std::wstring(dllPath) + L"\\gwca.dll";
    HMODULE hGwca = LoadLibraryW(gwcaPath.c_str());
    
    if (!hGwca) {
        MessageBoxW(NULL, L"找不到 gwca.dll,请确保它和本 DLL 在同目录！", L"错误", MB_ICONERROR);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        LoadGWCADependency(hModule);

        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            // 1. 初始化 JadeLogger (传入当前的 HMODULE)
            JadeLogger::Init(GetModuleHandleW(L"JadeTech_Bridge.dll")); 
            
            // 2. 【核心】初始化 GWCA，让内部指针生效
            JadeLogger::LogFile("SYSTEM", "正在初始化 GWCA...");
            if (!GW::Initialize()) {
                JadeLogger::LogFile("SYSTEM", "❌ GWCA 初始化失败！");
                return 1;
            }
            JadeLogger::LogFile("SYSTEM", "✅ GWCA 初始化成功！");

            // 3. 启动共享内存线程
            SharedMemoryThread(nullptr);
            return 0;
        }, nullptr, 0, nullptr);
    }
    return TRUE;
}
