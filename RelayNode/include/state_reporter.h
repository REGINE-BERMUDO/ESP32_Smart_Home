#pragma once
#include "esp_err.h"

// The callback sends a fresh reading, never a queued historical value.
esp_err_t setupStateReporter(void (*sendCurrentState)(void));
void requestStateReportRepeats(void);
