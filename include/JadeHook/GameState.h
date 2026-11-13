#pragma once
#include "HookState.h"
#include "../SharedMemory.h"
namespace JadeHook {
void UpdateGameState(SharedMemoryData* shared, RuntimeState* state);
}
