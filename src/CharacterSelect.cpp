#include <windows.h>
#include <cstdint>
#include <functional>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/Managers/MerchantMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include "../include/JadeHook/CharacterSelect.h"
#include "../include/JadeLogger.h"

namespace JadeHook {
// This mirrors the leading fields of GWToolbox++'s private CharSelectorContext.
// The context layout is not a public GWCA ABI; validate both pointer and frame id.
struct CharSelectorContextPrefix { uint32_t vtable; uint32_t frame_id; };

bool IsCharacterSelectReady() {
    uint32_t uiState = 10;
    if (!GW::UI::SendUIMessage(GW::UI::UIMessage::kCheckUIState, nullptr, &uiState))
        return false;
    if (uiState != 2) return false;

    auto* selector = GW::UI::GetFrameByLabel(L"Selector");
    if (!selector || !selector->IsCreated() || !selector->IsVisible()) return false;

    auto* ctx = static_cast<CharSelectorContextPrefix*>(GW::UI::GetFrameContext(selector));
    if (!ctx || reinterpret_cast<uintptr_t>(ctx) <= 0xFFFF) return false;
    return ctx->frame_id == selector->frame_id;
}

bool ClickPlayButton() {
    auto* play = GW::UI::GetFrameByLabel(L"Play");
    if (!play || !play->IsCreated() || !play->IsVisible() || play->IsDisabled()) {
        JadeLogger::LogFile("WARN", "Play frame missing, hidden, or disabled.");
        return false;
    }
    return GW::UI::ButtonClick(play);
}
}
