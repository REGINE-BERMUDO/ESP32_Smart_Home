# Host LDR task regression test

From the repository root, with a host GCC installed:

```powershell
gcc -std=c11 -Wall -Wextra -Wno-unused-parameter -I SensorNode/test/host/include SensorNode/test/host/test_ldr.c -o "$env:TEMP/ldr-test.exe"
& "$env:TEMP/ldr-test.exe"
```

This runs the actual `src/ldr.c` task with fake GPIO and FreeRTOS time. It checks
startup publication, stable ON/OFF changes, chatter rejection, change-only
publication, and failed-send retry spacing. Stub headers are used only by this
host command; they are outside the firmware source/include directories.

Gateway browser tests: `node --test GatewayNode/test/web_ui.test.cjs`.
Hardware acceptance: `docs/LIVE_STATE_TEST.md`.
