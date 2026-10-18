Run the focused state-report retry worker test from the repository root with a host C++ compiler:

```powershell
g++ -std=c++17 -Wall -Wextra -I RelayNode/test/host/include -I RelayNode/include RelayNode/test/host/test_state_reporter.cpp -o state-report-test.exe
./state-report-test.exe
```

The test executes the actual worker with simulated FreeRTOS notifications. It checks two bounded repeats, coalescing a newer state change, sending the latest value, and task creation failure. This does not simulate radio delivery. On hardware, confirm both web commands and sensor changes update the dashboard, including rapid alternation; a disconnected relay must never be shown as freshly confirmed.
