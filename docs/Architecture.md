# Architecture

- `dllmain.cpp`: minimal loader entry and exported lifecycle API.
- `SharedMemory.cpp`: worker loop, per-process shared memory, command dispatch.
- `GameState.cpp`: GWCA reads and game-thread state machine.
- `CharacterSelect.cpp`: character-select detection and Play-frame interaction.
- `JadeLogger.cpp`: file logging and optional GW chat output.

GWCA calls are scheduled through `GW::GameThread::Enqueue`; the worker thread only handles IPC and scheduling. At most one state task is queued at a time.
