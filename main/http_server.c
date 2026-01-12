#include <esp_event.h>
#include <esp_log.h>
#include <esp_check.h>
#include <esp_http_server.h>
#include "freertos/FreeRTOS.h"
#include <sys/time.h>

#include "struct_defs.h"
#include "http_server.h"
#include "http_literals.h"

static const char *TAG = "HTTPD";

httpd_handle_t server = NULL;
void (*activity_callback)(char, const char*);  // for callbacks to allow caller to monitor activity.


// ---------------- HTTP Server Start/Stop + handlers for HTTP methds + specs of URI endpoints + handlers for connect/disconnect (??) -------------

/* Index (and device timestamp) */
// The links are manually created here, rather than being generated when the URI handlers are added to the server.
static esp_err_t form_get_handler(httpd_req_t *req){
    (*activity_callback)('R', req->uri);  // signal a request was made

    // this is quite a large buffer because some of the "literal" chunks are quite large.
    // Higher values (e.g. 2000) end up causing stack protection faults when the snprintf includes floating point items 
    // (but the SPF can be prevented by using `config.stack_size = 6000` in the HTTP server setup)
    char resp_text[1500];
    
    snprintf(resp_text, sizeof(resp_text), "<html><title>Neopixel Wave</title>\n%s\n<body>\n<form method='post' action='/'>\n", style_l);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), cmap_l, channel_map);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), waveform_l,
        c1.waveform, c2.waveform, c3.waveform,
        c1.ratio, c2.ratio, c3.ratio);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), wavelength_l,
        c1.lambda, c2.lambda, c3.lambda,
        c1.phase, c2.phase, c3.phase);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), amplitude_l,
        c1.amplitude, c2.amplitude, c3.amplitude);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), am_l,
        c1.amp_mod_depth, c2.amp_mod_depth, c3.amp_mod_depth,
        c1.amp_mod_period_s, c2.amp_mod_period_s, c3.amp_mod_period_s,
        c1.amp_mod_ratio, c2.amp_mod_ratio, c3.amp_mod_ratio,
        c1.amp_mod_offset_s, c2.amp_mod_offset_s, c3.amp_mod_offset_s);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), velocity_l,
        c1.velocity, c2.velocity, c3.velocity);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), vm_l,
        c1.velocity_mod_depth, c2.velocity_mod_depth, c3.velocity_mod_depth,
        c1.velocity_mod_period_s, c2.velocity_mod_period_s, c3.velocity_mod_period_s,
        c1.velocity_mod_ratio, c2.velocity_mod_ratio, c3.velocity_mod_ratio,
        c1.velocity_mod_offset_s, c2.velocity_mod_offset_s, c3.velocity_mod_offset_s);
    httpd_resp_sendstr_chunk(req, resp_text);

    snprintf(resp_text, sizeof(resp_text), "<input type='submit' value='Save'/></form></body></html>");
    httpd_resp_sendstr_chunk(req, resp_text);
    httpd_resp_send_chunk(req, NULL, 0);  // signal end

    return ESP_OK;
}

static void do_char_param(const char * buff, const char * param_name, char * channel_param){
    char value[8];
    esp_err_t err = httpd_query_key_value(buff, param_name, value, sizeof(value));
    if ((strlen(value) > 0 ) && (err == ESP_OK)) {
        *channel_param = value[0];
    } else {
        ESP_LOGE(TAG, "Error processing param '%s'. Contained '%s'. Err '%s", param_name, value, esp_err_to_name(err));
    }
}

static void do_float_param(const char * buff, const char * param_name, float * channel_param){
    char value[16];
    esp_err_t err = httpd_query_key_value(buff, param_name, value, sizeof(value));
    if ((strlen(value) > 0 ) && (err == ESP_OK)) {
        *channel_param = atof(value);
    } else {
        ESP_LOGE(TAG, "Error processing param '%s'. Contained '%s'. Err '%s'", param_name, value, esp_err_to_name(err));
    }
}

static esp_err_t form_post_handler(httpd_req_t *req){
    (*activity_callback)('R', req->uri);

    int c_len = req->content_len;
    // ESP_LOGI(TAG, "req->conten_len = %u", c_len);
    if (c_len == 0){  // no content!
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No form data.");
        return ESP_FAIL;
    }

    // I HOPE  this is a small enough payload that it will never be chunked. BEWARE if scope of parameters is extended!!
    // Current (Jan 2026) payload size about 650 bytes. Could trim the parameter names.
    char buf[c_len + 1];  // add space for null termination, which is needed for reliable extraction of parameters
    httpd_req_recv(req, buf, sizeof(buf));
    buf[c_len] = 0;

    do_char_param(buf, "channel_map", &channel_map);

    do_char_param(buf, "waveform_1", &c1.waveform);
    do_char_param(buf, "waveform_2", &c2.waveform);
    do_char_param(buf, "waveform_3", &c3.waveform);
    
    do_float_param(buf, "lambda_1", &c1.lambda);
    do_float_param(buf, "lambda_2", &c2.lambda);
    do_float_param(buf, "lambda_3", &c3.lambda);

    do_float_param(buf, "phase_1", &c1.phase);
    do_float_param(buf, "phase_2", &c2.phase);
    do_float_param(buf, "phase_3", &c3.phase);
    
    do_float_param(buf, "ratio_1", &c1.ratio);
    do_float_param(buf, "ratio_2", &c2.ratio);
    do_float_param(buf, "ratio_3", &c3.ratio);

    do_float_param(buf, "amplitude_1", &c1.amplitude);
    do_float_param(buf, "amplitude_2", &c2.amplitude);
    do_float_param(buf, "amplitude_3", &c3.amplitude);

    do_float_param(buf, "am_depth_1", &c1.amp_mod_depth);
    do_float_param(buf, "am_depth_2", &c2.amp_mod_depth);
    do_float_param(buf, "am_depth_3", &c3.amp_mod_depth);
    do_float_param(buf, "am_period_1", &c1.amp_mod_period_s);
    do_float_param(buf, "am_period_2", &c2.amp_mod_period_s);
    do_float_param(buf, "am_period_3", &c3.amp_mod_period_s);
    do_float_param(buf, "am_ratio_1", &c1.amp_mod_ratio);
    do_float_param(buf, "am_ratio_2", &c2.amp_mod_ratio);
    do_float_param(buf, "am_ratio_3", &c3.amp_mod_ratio);
    do_float_param(buf, "am_offset_1", &c1.amp_mod_offset_s);
    do_float_param(buf, "am_offset_2", &c2.amp_mod_offset_s);
    do_float_param(buf, "am_offset_3", &c3.amp_mod_offset_s);

    do_float_param(buf, "velocity_1", &c1.velocity);
    do_float_param(buf, "velocity_2", &c2.velocity);
    do_float_param(buf, "velocity_3", &c3.velocity);

    do_float_param(buf, "vm_depth_1", &c1.velocity_mod_depth);
    do_float_param(buf, "vm_depth_2", &c2.velocity_mod_depth);
    do_float_param(buf, "vm_depth_3", &c3.velocity_mod_depth);
    do_float_param(buf, "vm_period_1", &c1.velocity_mod_period_s);
    do_float_param(buf, "vm_period_2", &c2.velocity_mod_period_s);
    do_float_param(buf, "vm_period_3", &c3.velocity_mod_period_s);
    do_float_param(buf, "vm_ratio_1", &c1.velocity_mod_ratio);
    do_float_param(buf, "vm_ratio_2", &c2.velocity_mod_ratio);
    do_float_param(buf, "vm_ratio_3", &c3.velocity_mod_ratio);
    do_float_param(buf, "vm_offset_1", &c1.velocity_mod_offset_s);
    do_float_param(buf, "vm_offset_2", &c2.velocity_mod_offset_s);
    do_float_param(buf, "vm_offset_3", &c3.velocity_mod_offset_s);
    
    // reload the form
    return form_get_handler(req);
}

static const httpd_uri_t home = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = form_get_handler
};

static const httpd_uri_t form_handler = {
    .uri       = "/",
    .method    = HTTP_POST,
    .handler   = form_post_handler
};

esp_err_t start_webserver(void (*callback)(char, const char*))
{
    #ifdef CONFIG_AP_LOG_LEVEL
    esp_log_level_set(TAG, CONFIG_AP_LOG_LEVEL);
    #else
    esp_log_level_set(TAG, ESP_LOG_WARN);
    #endif

    ESP_LOGI(TAG, "Starting Webserver");

    esp_err_t err = ESP_OK;

    server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Default is max 8 URI handlers
    config.max_uri_handlers = 4;

    config.stack_size = 5000;

    // Start the httpd server
    err = httpd_start(&server, &config);
    ESP_RETURN_ON_ERROR(err, TAG, "HTTPD start failed: %s", esp_err_to_name(err));
    // Set URI handlers
    err = httpd_register_uri_handler(server, &home);
    httpd_register_uri_handler(server, &form_handler);

    ESP_RETURN_ON_ERROR(err, TAG, "URI hander reg failed: %s", esp_err_to_name(err));

    ESP_LOGI(TAG, "Webserver started on port: '%d'", config.server_port);

    activity_callback = callback;
    (*activity_callback)('W', "-");
    
    return err;
}


esp_err_t stop_webserver(void){  // stops and garbage collects
    esp_err_t err = ESP_OK;
    if (server) {
        err = httpd_stop(server);
        if (err == ESP_OK) {
            // free(server);
            server = NULL;
            ESP_LOGI(TAG, "Stopped webserver");
        } else {
            ESP_LOGE(TAG, "Failed to stop http server: %s", esp_err_to_name(err));
        }
    }
    return err;
}
