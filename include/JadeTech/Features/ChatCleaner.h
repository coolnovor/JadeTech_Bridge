#pragma once

namespace JadeTech::Features::ChatCleaner
{
    // 初始化聊天清理模块。
    // 必须在 GWCA::Initialize() 成功后调用。
    bool Initialize();

    // 注销地图切换回调并停止后续清理任务。
    // 必须在 GW::Terminate() 之前调用。
    void Terminate();

    // 当前模块是否已成功启用。
    bool IsInitialized();
}