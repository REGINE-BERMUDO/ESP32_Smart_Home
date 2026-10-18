# Gateway regression checks

Browser logic, from the repository root:

```powershell
node --test GatewayNode/test/web_ui.test.cjs
```

The DOM/transport mocks exercise real `web/app.js`: command acceptance, failed
requests, state-driven selection, unavailable states, stream loss, reconnects,
silent streams, polling fallback, and late HTTP replies racing live events.

Async stream lifecycle, with host GCC/G++:

```powershell
g++ -std=c++17 -Wall -Wextra -Wno-unused-variable -I GatewayNode/test/host/include GatewayNode/test/host/test_stream.cpp -o "$env:TEMP/stream-test.exe"
& "$env:TEMP/stream-test.exe"
```

This compiles the actual `src/app_web.cpp` with fake HTTP/FreeRTOS APIs. It
checks bounded stream capacity, failed request/task allocations, initial and
changed states, heartbeats, and cleanup after a failed write. Test-only headers
are outside the firmware's source/include paths.

Radio latency, actual socket behavior, and physical relay operation require the
hardware checks in `docs/LIVE_STATE_TEST.md`; mocks cannot establish those.
