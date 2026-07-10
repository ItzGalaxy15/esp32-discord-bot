#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "estr.h"
#include "secrets.h"
#include "string.h"
#include "stdio.h"

static const char *TAG = "discord_bot";

static discord_handle_t bot;

#define TRIGGER_EMOJI_ID "1437286090494185653"
#define REPLY_EMOJI        "<:WawaWinted:1344699696341454889>"

static bool message_contains_emoji_id(const char *content, const char *emoji_id)
{
    if (!content || !emoji_id) {
        return false;
    }

    char needle[32];
    int written = snprintf(needle, sizeof(needle), ":%s>", emoji_id);
    if (written <= 0 || written >= (int)sizeof(needle)) {
        return false;
    }

    return strstr(content, needle) != NULL;
}

static void bot_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    discord_event_data_t *data = (discord_event_data_t *)event_data;

    switch (event_id) {
    case DISCORD_EVENT_CONNECTED: {
        discord_session_t *session = (discord_session_t *)data->ptr;

        ESP_LOGI(TAG, "Bot %s#%s connected", session->user->username, session->user->discriminator);
    } break;

    case DISCORD_EVENT_MESSAGE_RECEIVED: {
        discord_message_t *msg = (discord_message_t *)data->ptr;

        ESP_LOGI(TAG,
                 "New message (dm=%s, autor=%s#%s, bot=%s, channel=%s, guild=%s, content=%s)",
                 !msg->guild_id ? "true" : "false",
                 msg->author->username,
                 msg->author->discriminator,
                 msg->author->bot ? "true" : "false",
                 msg->channel_id,
                 msg->guild_id ? msg->guild_id : "NULL",
                 msg->content);

        if (msg->author->bot) {
            break;
        }

        if (message_contains_emoji_id(msg->content, TRIGGER_EMOJI_ID)) {
            ESP_LOGI(TAG, "Trigger emoji %s detected, replying with %s", TRIGGER_EMOJI_ID, REPLY_EMOJI);

            discord_message_t reply = { .content = REPLY_EMOJI, .channel_id = msg->channel_id };
            discord_message_t *sent_msg = NULL;
            esp_err_t err = discord_message_send(bot, &reply, &sent_msg);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Reply emoji sent");
                if (sent_msg) {
                    discord_message_free(sent_msg);
                }
            } else {
                ESP_LOGE(TAG, "Failed to send reply emoji (%s)", esp_err_to_name(err));
            }
            break;
        }

        char *echo_content = estr_cat("Hey ", msg->author->username, " you wrote ", msg->content);

        discord_message_t echo = { .content = echo_content, .channel_id = msg->channel_id };

        discord_message_t *sent_msg = NULL;
        esp_err_t err = discord_message_send(bot, &echo, &sent_msg);
        free(echo_content);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Echo message successfully sent");

            if (sent_msg) {
                ESP_LOGI(TAG, "Echo message got ID #%s", sent_msg->id);
                discord_message_free(sent_msg);
            }
        } else {
            ESP_LOGE(TAG, "Failed to send echo message");
        }
    } break;

    case DISCORD_EVENT_MESSAGE_UPDATED: {
        discord_message_t *msg = (discord_message_t *)data->ptr;
        ESP_LOGI(TAG,
                 "%s updated message #%s. New content: %s",
                 msg->author->username,
                 msg->id,
                 msg->content);
    } break;

    case DISCORD_EVENT_MESSAGE_DELETED: {
        discord_message_t *msg = (discord_message_t *)data->ptr;
        ESP_LOGI(TAG, "Message #%s deleted", msg->id);
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

    discord_config_t cfg = {
        .intents = DISCORD_INTENT_GUILD_MESSAGES | DISCORD_INTENT_MESSAGE_CONTENT,
        .token = DISCORD_BOT_TOKEN,
    };

    bot = discord_create(&cfg);
    ESP_ERROR_CHECK(discord_register_events(bot, DISCORD_EVENT_ANY, bot_event_handler, NULL));
    ESP_ERROR_CHECK(discord_login(bot));
}
