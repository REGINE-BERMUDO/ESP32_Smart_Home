#include "state_reporter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t reportTask = nullptr;
static void (*sendCurrentState)(void) = nullptr;

static void repeatStateReports(void *)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        unsigned repeats = 0;
        while (repeats < 2)
        {
            // A new change restarts the short burst. Do not queue old states.
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(150)) != 0)
            {
                repeats = 0;
                continue;
            }
            sendCurrentState();
            ++repeats;
        }
    }
}

esp_err_t setupStateReporter(void (*send)(void))
{
    if (send == nullptr) return ESP_ERR_INVALID_ARG;
    sendCurrentState = send;
    return xTaskCreate(repeatStateReports, "relay_reports", 3072, nullptr, 5, &reportTask) == pdPASS
        ? ESP_OK : ESP_ERR_NO_MEM;
}

void requestStateReportRepeats(void)
{
    if (reportTask != nullptr) xTaskNotifyGive(reportTask);
}
