idf_component_register(SRCS "discord_time_bot.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp-discord protocol_examples_common esp_netif nvs_flash esp_event)