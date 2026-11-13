#include <windows.h>

#include <cstdint>
#include <functional>

#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/Managers/MerchantMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include "JadeTech/Features/CharacterSelect.h"
#include "JadeTech/Core/JadeLogger.h"

namespace JadeHook {

// 对 GWToolbox++ 私有 CharSelectorContext 前部字段的镜像。
// 该结构不是 GWCA 公开 ABI，使用前需要验证指针和 Frame ID。
struct CharSelectorContextPrefix {
    uint32_t vtable;
    uint32_t frame_id;
};

bool IsCharacterSelectReady() {
    uint32_t uiState = 10;

    if (!GW::UI::SendUIMessage(GW::UI::UIMessage::kCheckUIState, nullptr,&uiState)) {
        return false;
    }

    if (uiState != 2) {
        return false;
    }

    auto* selector = GW::UI::GetFrameByLabel(L"Selector");

    if (!selector || !selector->IsCreated() || !selector->IsVisible()) {
        return false;
    }

    auto* ctx = static_cast<CharSelectorContextPrefix*>(
        GW::UI::GetFrameContext(selector));

    if (!ctx || reinterpret_cast<uintptr_t>(ctx) <= 0xFFFF) {
        return false;
    }

    return ctx->frame_id == selector->frame_id;
}

bool ClickPlayButton() {
    // 点击前再次确认人物选择界面
    if (!IsCharacterSelectReady()) {JadeLogger::LogFile("WARN", "当前不处于有效的人物选择界面，取消点击。");
        return false;
    }

    auto* play = GW::UI::GetFrameByLabel(L"Play");

    if (!play || !play->IsCreated() || !play->IsVisible() ||
        play->IsDisabled()) {
        JadeLogger::LogFile("WARN","Play 按钮不存在、不可见或不可用，取消点击。");
        return false;
    }

    JadeLogger::LogFile("SYSTEM", "人物选择界面确认，准备点击 Play。");

    return GW::UI::ButtonClick(play);
}

}  // namespace JadeHook
