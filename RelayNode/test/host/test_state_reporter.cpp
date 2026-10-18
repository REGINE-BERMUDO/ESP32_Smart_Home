#include <cassert>
#include <cstdio>
#include <vector>
#include "../../src/state_reporter.cpp"
struct EndTask {};
static void (*worker)(void *);
static bool createFails, currentState = true;
static unsigned notifications;
static std::vector<bool> sent;
static std::vector<unsigned> waits;
static std::vector<int> events;
int xTaskCreate(void (*task)(void *), const char *, unsigned, void *, unsigned, TaskHandle_t *handle) {
    worker = task;
    if (createFails) return 0;
    *handle = reinterpret_cast<void *>(1);
    return pdPASS;
}
void xTaskNotifyGive(TaskHandle_t handle) { assert(handle); ++notifications; }
unsigned ulTaskNotifyTake(int clear, TickType_t delay) {
    assert(clear == pdTRUE);
    waits.push_back(delay);
    if (events.empty()) throw EndTask{};
    const int event = events.front(); events.erase(events.begin());
    if (event == 2) { currentState = false; return 1; }
    return event;
}
static void sendCurrent() { sent.push_back(currentState); }
static void run() { try { worker(nullptr); } catch (EndTask &) {} }
int main() {
    assert(setupStateReporter(nullptr) == ESP_ERR_INVALID_ARG);
    createFails = true;
    assert(setupStateReporter(sendCurrent) == ESP_ERR_NO_MEM);
    requestStateReportRepeats();
    assert(notifications == 0);
    createFails = false;
    assert(setupStateReporter(sendCurrent) == ESP_OK);
    requestStateReportRepeats();
    assert(notifications == 1);
    events = {1, 0, 0}; run();
    assert((sent == std::vector<bool>{true, true}));
    assert((waits == std::vector<unsigned>{portMAX_DELAY, 150, 150, portMAX_DELAY}));
    sent.clear(); waits.clear();
    // A newer OFF notification supersedes repeats of an older ON.
    currentState = true; events = {1, 0, 2, 0, 0}; run();
    assert((sent == std::vector<bool>{true, false, false}));
    assert(waits.back() == portMAX_DELAY); // No continuous background retransmission.
    std::puts("State reporter: bounded retries, latest state, coalescing, and setup failure passed.");
}
