#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "discord.h"
#include "discord/session.h"
#include "discord/presence.h"
#include "secrets.h"

static const char *TAG = "discord_time_bot";

static discord_handle_t bot;
static TaskHandle_t presence_task;

#define PRESENCE_UPDATE_INTERVAL_MS 60000

static void format_eu_time(char *buf, size_t buflen)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, buflen, "%d/%m %H:%M %Z", &timeinfo);
}

static void update_presence(void)
{
    char status[32];
    format_eu_time(status, sizeof(status));

    esp_err_t err = discord_presence_set(bot, status);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Presence updated: %s", status);
    } else {
        ESP_LOGE(TAG, "Failed to update presence: %s (%s)", status, esp_err_to_name(err));
    }
}

static void presence_update_task(void *arg)
{
    (void)arg;

    while (true) {
        update_presence();
        vTaskDelay(pdMS_TO_TICKS(PRESENCE_UPDATE_INTERVAL_MS));
    }
}

static void time_sync_init(void)
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

static void wait_for_time_sync(void)
{
    struct tm timeinfo = { 0 };

    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(500));
        time_t now = 0;
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    char buf[32];
    format_eu_time(buf, sizeof(buf));
    ESP_LOGI(TAG, "Time synced: %s", buf);
}

static void bot_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    discord_event_data_t *data = (discord_event_data_t *)event_data;

    switch (event_id) {
    case DISCORD_EVENT_CONNECTED: {
        discord_session_t *session = (discord_session_t *)data->ptr;
        ESP_LOGI(TAG, "Bot %s#%s connected", session->user->username, session->user->discriminator);

        if (presence_task == NULL) {
            xTaskCreate(presence_update_task, "presence", 4096, NULL, 5, &presence_task);
        } else {
            update_presence();
        }
    } break;

    case DISCORD_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Bot logged out");
        break;

    default:
        break;
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    time_sync_init();
    wait_for_time_sync();

    discord_config_t cfg = {
        .intents = DISCORD_INTENT_GUILDS,
        .token = DISCORD_BOT_TOKEN,
    };

    bot = discord_create(&cfg);
    ESP_ERROR_CHECK(discord_register_events(bot, DISCORD_EVENT_ANY, bot_event_handler, NULL));
    ESP_ERROR_CHECK(discord_login(bot));
}
