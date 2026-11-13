#include <windows.h>
#include <functional>
#include <string>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include "../include/JadeHook/GameState.h"
#include "../include/JadeHook/CharacterSelect.h"
#include "../include/JadeLogger.h"

namespace JadeHook {
constexpr ULONGLONG kRetryMs = 2000;
constexpr ULONGLONG kWelcomeDelayMs = 1000;

void UpdateGameState(SharedMemoryData* shared, RuntimeState* state) {
    if (!shared || !state || !GW::GameThread::IsInGameThread()) return;
    const ULONGLONG now = GetTickCount64();
    const bool loaded = GW::Map::GetIsMapLoaded();
    const DWORD mapId = static_cast<DWORD>(GW::Map::GetMapID());
    const DWORD instance = static_cast<DWORD>(GW::Map::GetInstanceType());

    // Shared memory is a cross-process ABI; aligned DWORD writes are atomic on x86.
    shared->IsMapLoaded = loaded ? 1u : 0u;
    shared->MapId = mapId;
    shared->InstanceType = instance;

    if (now - state->lastStatusLog >= 3000) {
        JadeLogger::LogFile("STATUS", "loaded=" + std::to_string(loaded) +
            ", instance=" + std::to_string(instance) + ", map=" + std::to_string(mapId));
        state->lastStatusLog = now;
    }

    if (loaded) {
        if (!state->lastMapLoaded) {
            state->mapLoadedTime = now;
            state->enterState = EnterState::Completed;
            JadeLogger::LogFile("SYSTEM", "Map loaded; character entry confirmed.");
        }
        if (!state->welcomed && now - state->mapLoadedTime >= kWelcomeDelayMs) {
            state->welcomed = true;
            JadeLogger::Info(L"欢迎使用 JadeTech 翠玉科技！", false);
        }
    } else {
        if (state->lastMapLoaded) {
            state->mapLoadedTime = 0;
            state->welcomed = false;
            state->enterState = EnterState::Waiting;
        }
        if (IsCharacterSelectReady()) {
            state->enterState = EnterState::CharacterSelect;
            if (now - state->lastEnterAttempt >= kRetryMs) {
                state->lastEnterAttempt = now;
                JadeLogger::LogFile("SYSTEM", "Character select ready; requesting Play.");
                if (ClickPlayButton()) {
                    state->enterState = EnterState::RequestSent;
                    JadeLogger::LogFile("SYSTEM", "Play click accepted; waiting for map load.");
                }
            }
        } else if (state->enterState != EnterState::RequestSent) {
            state->enterState = EnterState::Waiting;
        }
    }
    state->lastMapLoaded = loaded;
}
}
