# JadeTech_Bridge
JadeTech 翠玉科技 - 内部触手 C++ DLL

32-bit Windows DLL bridge using the bundled GWCA import library/runtime.

## Build

The project is x86-only because the bundled GWCA headers contain a 32-bit ABI assertion and the supplied `gwca.dll` is PE32/i386.

```bash
./scripts/setup-toolchain.sh
./scripts/build-xwin.sh
```

The resulting `JadeTech_Bridge.dll` and `gwca.dll` are placed together in `build-xwin/`.

## Injector contract

After loading `JadeTech_Bridge.dll` into the Guild Wars process, call the exported function:

- `JadeTech_Initialize()` (`BOOL __stdcall`)
- `JadeTech_Shutdown(timeout_ms)` (`BOOL __stdcall`)

Do not expect DllMain to start GWCA. Initialization is intentionally explicit to avoid doing complex work under the Windows loader lock.

## Shared memory

Name: `Local\\JadeTech_Bridge_SharedMem_<PID>`

The controller must write `ChatMessage`, then `CommandId=1`, and wait until `Status=2` before submitting another command. The structure is in `include/SharedMemory.h`; keep the Easy Language structure byte-for-byte compatible.

## Important limits

- `gwca.dll` and `JadeTech_Bridge.dll` must both be x86 and from compatible GWCA builds.
- Character selector context inspection mirrors a private GWToolbox++ context prefix; it is not a stable public GWCA ABI. Revalidate it when updating GWCA/client versions.
- Build success does not prove runtime initialization or game UI behavior. Verify `JadeTech.log` and test with one client first.
