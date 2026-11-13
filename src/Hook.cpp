#include <windows.h>
#include "../include/JadeTech/HookState.h"

namespace JadeHook {
BridgeRuntime& GetRuntime() { static BridgeRuntime runtime; return runtime; }
}
