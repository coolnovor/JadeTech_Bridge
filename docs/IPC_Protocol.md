# IPC protocol v1

Shared memory name: `Local\\JadeTech_Bridge_SharedMem_<PID>`.

`Magic=0x4A616465`, `ProtocolVersion=1`.

Command slot v1:
1. Controller writes the UTF-16 message into `ChatMessage`.
2. Controller writes `CommandId=1` and `Status=0`.
3. Bridge sets `Status=1`, processes the message, clears `CommandId`, then sets `Status=2`.
4. Controller waits for `Status=2` before reusing the slot.

This is a single-command mailbox, not a multi-producer queue. Add a mutex or sequence-based ring buffer before allowing concurrent writers.
