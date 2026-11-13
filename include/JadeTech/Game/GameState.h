#pragma once
#include "JadeTech/Runtime/HookState.h"
#include "JadeTech/IPC/SharedMemory.h"
namespace JadeHook {
void UpdateGameState(SharedMemoryData* shared, RuntimeState* state);
}
