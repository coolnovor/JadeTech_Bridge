#include <windows.h>
#include "../include/JadeHook/HookState.h"

namespace JadeHook {
BridgeRuntime& GetRuntime() { static BridgeRuntime runtime; return runtime; }
}
