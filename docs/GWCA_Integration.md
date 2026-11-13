# GWCA integration

The bundle contains `Dependencies/GWCA/include`, `lib/gwca.lib`, and `bin/gwca.dll`.
CMake defines `GWCA_IMPORT`, links `gwca.lib`, and copies `gwca.dll` beside the output Bridge DLL.

The character selector check is based on the GWToolbox++ implementation pattern: UI state `kCheckUIState == 2`, visible `Selector` frame, and matching `frame_id` in the leading context fields. The context type is private to GWToolbox++ and is not a guaranteed public GWCA contract.
