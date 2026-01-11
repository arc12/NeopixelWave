idf_component_register(SRCS "http_server.c" "ap_server.c" "main.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_http_server nvs_flash)