# NEXT - M3.2 (Binary writes to channel)

## Goal
Enable binary to send EGL commands to wrapper via mce_cmd.bin.

## Blocker
Termux OOM when rebuilding Client.
Previous OOM: killed at ~1 minute of compile.

## Solutions
1. ccache (pkg install ccache)
   - caches compile results
   - reduces rebuild time on repeated tries
2. Build one .cpp at a time
   - clang++ manually with exact flags
   - bypass CMake archive
3. Split Linux_Globals.cpp into smaller units
4. Use -O0 -g0 -fno-exceptions to reduce RAM

## Plan
1. Install ccache
2. Configure CMake to use ccache
3. Rebuild in background (nohup)
4. If OOM: rebuild with -O0 only
5. Test: binary sends red color on start

## Success Criteria
- MCE-stdout.txt shows: "[bridge] CMD channel attached"
- MCE-stdout.txt shows: "[bridge] Test RED sent"
- Screen turns RED for 2 seconds, then green (exit 0)

## Already prepared
- Linux_Globals.cpp modified with bridge code (line 1-50)
- Test send added in constructor(150)
- Ready for rebuild
