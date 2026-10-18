// Run from the repository root:
// gcc -std=c11 -Wall -Wextra -Wno-unused-parameter -I SensorNode/test/host/include SensorNode/test/host/test_ldr.c -o <temporary-executable>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../../src/ldr.c"

static jmp_buf taskExit;
static bool samples[100];
static unsigned steps, step, calls, failures;
static unsigned publishedAt[10];
static bool published[10];
static TickType_t ticks;

int gpio_get_level(int pin) { assert(pin == 27); return samples[step]; }
int gpio_config(const gpio_config_t *config) { return ESP_OK; }
TickType_t xTaskGetTickCount(void) { return ticks; }
void xTaskDelayUntil(TickType_t *reference, TickType_t delay)
{
    assert(delay == 50);
    *reference += delay;
    ticks += delay;
    if (++step >= steps) longjmp(taskExit, 1);
}
int xTaskCreate(void (*task)(void *), const char *name, unsigned stack, void *arg, unsigned priority, void *handle) { return pdPASS; }
int appble_update_state(bool state)
{
    assert(calls < 10);
    publishedAt[calls] = ticks;
    published[calls++] = state;
    if (failures) { --failures; return 1; }
    return ESP_OK;
}
static void run(unsigned count, unsigned failedAttempts)
{
    steps = count; step = calls = 0; ticks = 0; failures = failedAttempts;
    atomic_store(&ldrState_Prev, false);
    if (!setjmp(taskExit)) readLDR(NULL);
}
int main(void)
{
    memset(samples, 1, sizeof(samples));
    run(100, 0);
    assert(calls == 1 && published[0] && publishedAt[0] == 150);
    assert(sendLDR()); // An unchanged sensor cannot repeatedly undo a web override.

    memset(samples, 0, sizeof(samples));
    for (unsigned i = 10; i < 20; ++i) samples[i] = true;
    run(30, 0);
    assert(calls == 3 && !published[0] && published[1] && !published[2]);
    assert(publishedAt[1] == 650 && publishedAt[2] == 1150);

    for (unsigned i = 0; i < 20; ++i) samples[i] = i % 2;
    for (unsigned i = 20; i < 40; ++i) samples[i] = true;
    run(40, 0);
    assert(calls == 1 && published[0] && publishedAt[0] >= 1100);

    memset(samples, 1, sizeof(samples));
    run(40, 2);
    assert(calls == 3 && publishedAt[0] == 150 && publishedAt[1] == 650 && publishedAt[2] == 1150);
    puts("LDR task: stable startup, light changes, chatter rejection, and bounded retries passed.");
}
