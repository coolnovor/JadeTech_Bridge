#include <windows.h>
#include <functional>
#include <string>
#include "../include/JadeHook/HookState.h"
#include "../include/JadeHook/SharedMemory.h"
#include "../include/JadeLogger.h"
#include <GWCA/GWCA.h>

namespace {
DWORD WINAPI WorkerMain(LPVOID) {
    auto& rt = JadeHook::GetRuntime();
    JadeLogger::Init(rt.module);
    JadeLogger::LogFile("SYSTEM", "JadeTech_Bridge worker starting.");

    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(rt.module, path, MAX_PATH)) {
        JadeLogger::LogFile("ERROR", "Cannot resolve Bridge module path.");
        rt.running.store(false); return 1;
    }
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) { rt.running.store(false); return 2; }
    *(slash + 1) = L'\0';
    std::wstring gwcaPath = std::wstring(path) + L"gwca.dll";
    HMODULE gwca = LoadLibraryW(gwcaPath.c_str());
    if (!gwca) {
        JadeLogger::LogFile("ERROR", "LoadLibrary gwca.dll failed: " + std::to_string(GetLastError()));
        rt.running.store(false); return 3;
    }

    if (!GW::Initialize()) {
        JadeLogger::LogFile("ERROR", "GWCA::Initialize failed.");
        FreeLibrary(gwca); rt.running.store(false); return 4;
    }
    JadeLogger::LogFile("SYSTEM", "GWCA initialized.");
    const DWORD result = JadeHook::SharedMemoryThread(nullptr);
    GW::DisableHooks();
    GW::Terminate();
    FreeLibrary(gwca);
    rt.running.store(false);
    JadeLogger::LogFile("SYSTEM", "JadeTech_Bridge worker stopped.");
    return result;
}
}

// DllMain deliberately does not initialize GWCA or create threads under loader lock.
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        JadeHook::GetRuntime().module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall JadeTech_Initialize() {
    auto& rt = JadeHook::GetRuntime();
    bool expected = false;
    if (!rt.running.compare_exchange_strong(expected, true)) return TRUE;
    rt.stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    rt.worker = CreateThread(nullptr, 0, WorkerMain, nullptr, 0, nullptr);
    if (!rt.worker) {
        rt.running.store(false);
        if (rt.stopEvent) { CloseHandle(rt.stopEvent); rt.stopEvent = nullptr; }
        return FALSE;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall JadeTech_Shutdown(DWORD timeoutMs) {
    auto& rt = JadeHook::GetRuntime();
    rt.running.store(false);
    if (rt.worker) {
        const DWORD wait = WaitForSingleObject(rt.worker, timeoutMs);
        if (wait != WAIT_OBJECT_0) return FALSE;
        CloseHandle(rt.worker); rt.worker = nullptr;
    }
    if (rt.stopEvent) { CloseHandle(rt.stopEvent); rt.stopEvent = nullptr; }
    return TRUE;
}
