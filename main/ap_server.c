#include <stdio.h>
// Prune these ??
#include <esp_wifi.h>
#include "esp_wifi_types.h"
#include <nvs_flash.h>
#include "esp_netif.h"
#include "esp_mac.h"  // only needed for showing MAC when STA connects.
#include <esp_log.h>
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "string.h"

#include <sys/time.h>

#include "ap_server.h"

#define MAX_STA_CONN 1  // one connection and the http server starts and stops on connect and disconnect to the AP

static const char* TAG = "AP";

time_t ap_last_event_time;
char ap_last_event_code = 'N';

void ws_activity_callback(char event_code, const char* url){
    ap_last_event_code = event_code;
    time(&ap_last_event_time);
    ESP_LOGI(TAG, "WS Callback %c - %s", event_code, url);
}

//---------------------------------------  WiFi setup and event handling ---------------------------------------

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    // ESP_LOGI(TAG, "WiFi event %li", event_id);
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        ap_last_event_code = 'C';
        time(&ap_last_event_time);
        start_webserver(ws_activity_callback);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
        stop_webserver();
        ap_last_event_code = 'D';
        time(&ap_last_event_time);
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    // ESP_LOGI(TAG, "IP event %li", event_id);
    if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        // added this to get a log message even when wifi component logging set to ERROR (to mitigate logging spam)
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; 
        ESP_LOGI(TAG, "Assigned IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}


//------------------------------------------------------------------------------

void begin_ap_server(){
    #ifdef CONFIG_AP_LOG_LEVEL
    esp_log_level_set(TAG, CONFIG_AP_LOG_LEVEL);
    #else
    esp_log_level_set(TAG, ESP_LOG_WARN);
    #endif
    if (ap_last_event_code == 'N'){
        // ESP_LOGI(TAG, "NVS and NetIFinit");
        ESP_RETURN_VOID_ON_ERROR(nvs_flash_init(), TAG, "NVS init failed");
        ESP_RETURN_VOID_ON_ERROR(esp_netif_init(), TAG, "NETIF init failed");
        
        // ESP_LOGI(TAG, "Eventloop create");
        ESP_RETURN_VOID_ON_ERROR(esp_event_loop_create_default(), TAG, "Eventloop create failed");
    
        // ESP_LOGI(TAG, "init softAP");
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_RETURN_VOID_ON_ERROR(esp_wifi_init(&cfg), TAG, "WiFi init failed");
        ESP_RETURN_VOID_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL), TAG, "Reg WIFI_EVENT failed");
        // added for IP assignment event
        ESP_RETURN_VOID_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL), TAG, "Reg IP_EVENT failed");  // IP_EVENT_STA_GOT_IP

        wifi_config_t wifi_config = {
            .ap = {
                // .ssid = {*ssid},
                // .ssid_len = strlen(ssid),
                .password = CONFIG_AP_PASSWORD,
                .max_connection = MAX_STA_CONN,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                .channel = CONFIG_AP_CHANNEL
            },
        };
        // although the wifi_config.ap.ssid property can be set with a constant in the previous command, it is not possible to use a dynamically-constructed string there
        // so... some jiggery pokery
        char ssid[32] = "Neopixel_Wave";
        strncpy((char *)wifi_config.ap.ssid, ssid, 32);

        if (strlen(CONFIG_AP_PASSWORD) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_RETURN_VOID_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "WiFi set mode failed");
        ESP_RETURN_VOID_ON_ERROR(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_BW20), TAG, "WiFi set bandwidth failed");  // narrow bandwidth
        ESP_RETURN_VOID_ON_ERROR(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config), TAG, "WiFi set config failed");
        ESP_RETURN_VOID_ON_ERROR(esp_wifi_start(), TAG, "WiFi start failed");
        ESP_RETURN_VOID_ON_ERROR(esp_wifi_set_max_tx_power(CONFIG_AP_POWER), TAG, "Wifi set power failed");

        ESP_LOGI(TAG, "SoftAP Created. SSID:%s password:%s", ssid, CONFIG_AP_PASSWORD);
        ap_last_event_code = 'A';
        time(&ap_last_event_time);
    }
}

void end_ap_server(){
    if (ap_last_event_code != 'N'){
        ap_last_event_code = 'N';
        stop_webserver();
    }
}