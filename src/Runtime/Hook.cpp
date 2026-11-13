#include <windows.h>
#include "JadeTech/Runtime/HookState.h"

namespace JadeHook {
BridgeRuntime& GetRuntime() { static BridgeRuntime runtime; return runtime; }
}
