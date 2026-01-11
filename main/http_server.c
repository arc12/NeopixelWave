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
        ESP_LOGE(TAG, "Error processing param '%s'. Contained '%s'. Err '%s", channel_param, value, esp_err_to_name(err));
    }
}

static void do_float_param(const char * buff, const char * param_name, float * channel_param){
    char value[16];
    esp_err_t err = httpd_query_key_value(buff, param_name, value, sizeof(value));
    if ((strlen(value) > 0 ) && (err == ESP_OK)) {
        *channel_param = atof(value);
    } else {
        ESP_LOGE(TAG, "Error processing param '%s'. Contained '%s'. Err '%s", channel_param, value, esp_err_to_name(err));
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

    do_float_param(buf, "amp_mod_depth_1", &c1.amp_mod_depth);
    do_float_param(buf, "amp_mod_depth_2", &c2.amp_mod_depth);
    do_float_param(buf, "amp_mod_depth_3", &c3.amp_mod_depth);
    do_float_param(buf, "amp_mod_period_1", &c1.amp_mod_period_s);
    do_float_param(buf, "amp_mod_period_2", &c2.amp_mod_period_s);
    do_float_param(buf, "amp_mod_period_3", &c3.amp_mod_period_s);
    do_float_param(buf, "amp_mod_ratio_1", &c1.amp_mod_ratio);
    do_float_param(buf, "amp_mod_ratio_2", &c2.amp_mod_ratio);
    do_float_param(buf, "amp_mod_ratio_3", &c3.amp_mod_ratio);
    do_float_param(buf, "amp_mod_offset_1", &c1.amp_mod_offset_s);
    do_float_param(buf, "amp_mod_offset_2", &c2.amp_mod_offset_s);
    do_float_param(buf, "amp_mod_offset_3", &c3.amp_mod_offset_s);

    do_float_param(buf, "velocity_1", &c1.velocity);
    do_float_param(buf, "velocity_2", &c2.velocity);
    do_float_param(buf, "velocity_3", &c3.velocity);

    do_float_param(buf, "velocity_mod_depth_1", &c1.velocity_mod_depth);
    do_float_param(buf, "velocity_mod_depth_2", &c2.velocity_mod_depth);
    do_float_param(buf, "velocity_mod_depth_3", &c3.velocity_mod_depth);
    do_float_param(buf, "velocity_mod_period_1", &c1.velocity_mod_period_s);
    do_float_param(buf, "velocity_mod_period_2", &c2.velocity_mod_period_s);
    do_float_param(buf, "velocity_mod_period_3", &c3.velocity_mod_period_s);
    do_float_param(buf, "velocity_mod_ratio_1", &c1.velocity_mod_ratio);
    do_float_param(buf, "velocity_mod_ratio_2", &c2.velocity_mod_ratio);
    do_float_param(buf, "velocity_mod_ratio_3", &c3.velocity_mod_ratio);
    do_float_param(buf, "velocity_mod_offset_1", &c1.velocity_mod_offset_s);
    do_float_param(buf, "velocity_mod_offset_2", &c2.velocity_mod_offset_s);
    do_float_param(buf, "velocity_mod_offset_3", &c3.velocity_mod_offset_s);

    // char value[16];
    // err = httpd_query_key_value(buf, "amplitude_1", value, sizeof(value));
    // if ((strlen(value) > 0 ) && (err == ESP_OK)) {
    //     c1.amplitude = atof(value);
    // } else {
    //     ESP_LOGE(TAG, "Error processing param '%s'. Contained '%s'. Err '%s", "amplitude_1", value, esp_err_to_name(err));
    // }

    // char return_page[16] = "settings";
    // char key[16];
    // char value[SETTINGS_CO_BUFF_LEN];
    // char c[2];
    // uint8_t component;
    // // return_page allows for NVS KV setting of calibration parameters (etc) which are POSTED from different pages
    // httpd_query_key_value(buf, "return_page", return_page, sizeof(key));
    // // Get key & value of expected key - format as query-string
    // if (httpd_query_key_value(buf, "key", key, sizeof(key)) == ESP_OK) {
    //     err = httpd_query_key_value(buf, "value", value, sizeof(value));
    //     if ((strlen(value) > 0 ) && (err == ESP_OK)) {
    //         if (httpd_query_key_value(buf, "c", c, sizeof(c)) == ESP_OK) {
    //             component = atoi(c);
    //         } else {
    //             httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing component");
    //             return ESP_FAIL;    
    //         }
    //     } else {
    //         if (err == ESP_ERR_HTTPD_RESULT_TRUNC) {
    //             httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Value was too long (exceeds SETTINGS_CO_BUFF_LEN)");
    //         } else {
    //             httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing value");
    //         }
    //         return ESP_FAIL;
    //     }
    // } else {
    //     httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing key");
    //     return ESP_FAIL;
    // }

    // app_settings_source_t ass = app_settings_sources[component];
    // err = ass.settings_store_str_fn(key, value);
    // switch (err)
    // {
    //     case ESP_OK:
    //         break;
    //     case ESP_ERR_INVALID_ARG:
    //         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unsupported key parameter");
    //         return ESP_FAIL;
    //     default:
    //         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
    //         return ESP_FAIL;
    // }

    // // read from NVS for hopefully-confirmatory msg
    // char o[SETTINGS_CO_BUFF_LEN];
    // ass.settings_get_str_fn(key, value, o);
    // char resp_buff[200];
    // snprintf(resp_buff, sizeof(resp_buff), "Set %s to %s (default %s). <a href=\"/%s?c=%d\">Return</a>.", key, value, o, return_page, component);
    
    // httpd_resp_sendstr_chunk(req, style_l);
    // httpd_resp_sendstr_chunk(req, resp_buff);
    // httpd_resp_send_chunk(req, NULL, 0);  // end chunks
    
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

// static esp_err_t settings_get_handler(httpd_req_t *req){
//     (*activity_callback)('R', req->uri);

//     // vars for query params
//     int component = -1;
//     char key[16] = {0};
//     char value[SETTINGS_CO_BUFF_LEN] = {0};

//     /* Read URL query string length and allocate memory for length + 1,
//      * extra byte for null termination */
//     size_t q_len = httpd_req_get_url_query_len(req) + 1;
//     if (q_len > 0){
//         char buf[q_len];
//         char param[SETTINGS_CO_BUFF_LEN];
//         if (httpd_req_get_url_query_str(req, buf, q_len) == ESP_OK) {
//             // component
//             if (httpd_query_key_value(buf, "c", param, sizeof(param)) == ESP_OK) {
//                 component = atoi(param);
//             }
//             // key & value
//             httpd_query_key_value(buf, "key", key, sizeof(key));
//             httpd_query_key_value(buf, "value", value, sizeof(value));
//         }
//     }
    
//     httpd_resp_sendstr_chunk(req, style_l);

//     // list components as links
//     uint out_buff_len = app_settings_sources_size * 75 + 30;
//     char out_buff[out_buff_len];
//     size_t index = snprintf(out_buff, out_buff_len, "<H1>Settings</H1>\n<ul>\n");
//     for (uint8_t i = 0; i< app_settings_sources_size; i++){
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li><a href=\"/settings?c=%d\">%s</a></li>\n", i, app_settings_sources[i].source_name);
//     }
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "</ul>\n");
//     httpd_resp_send_chunk(req, (const char *)out_buff, index);
    
//     // list settings for component if selected, in form of POSTable form
//     if (component >= 0){
//         app_settings_source_t ass = app_settings_sources[component];
//         char form_buf[200];
//         uint form_buf_len = sizeof(form_buf);
//         snprintf(form_buf, form_buf_len, "<H2>%s</H2>\n<form action=\"/settings\" method=\"post\"><input type=\"hidden\" name=\"c\" value=\"%d\">\n<select name=\"key\">\n",
//             ass.source_name, component);
//         httpd_resp_sendstr_chunk(req, (const char *)form_buf);
//         const char* key;
//         char c[SETTINGS_CO_BUFF_LEN];
//         char o[SETTINGS_CO_BUFF_LEN];
//         for (int i=0; i<ass.n_settings; i++){
//             key = ((const char **)ass.settings_available_ptr)[i];
//             ass.settings_get_str_fn(key, c, o);
//             snprintf(form_buf, form_buf_len, "<option value=\"%s\">%s (current=%s, default=%s)</option>", key, key, c, o);
//             httpd_resp_sendstr_chunk(req, (const char *)form_buf);
//         }
//         snprintf(form_buf, form_buf_len, "</select>\n<input type=\"text\" name=\"value\">\n<input type=\"submit\" value=\"Set\">\n</form>");
//         httpd_resp_sendstr_chunk(req, (const char *)form_buf);

//         // sensor components may have current readings info to inform setting value choice
//         if (ass.calibration_info_fn != NULL) {
//             ass.calibration_info_fn(form_buf, form_buf_len);
//             httpd_resp_sendstr_chunk(req, (const char*) form_buf);
//         }

//         // compiled config
//         if (ass.config_report != NULL) {
//             httpd_resp_sendstr_chunk(req, "<H2>Compiled Config</H2>");
//             httpd_resp_sendstr_chunk(req, *ass.config_report);
//         }
//     }
    
//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);  // end chunks

//     return ESP_OK;
// }

// // simple dump of "all" NVS settings (at least those under app control): settings, calibration data, internal persistent variables
// // Settings are automatically obtained from the "app_settings_sources" but there are some explicit extras too.
// static esp_err_t nvs_dump_get_handler(httpd_req_t *req){
//     (*activity_callback)('R', req->uri);

//     char current[SETTINGS_CO_BUFF_LEN];
//     char original[SETTINGS_CO_BUFF_LEN];

//     httpd_resp_sendstr_chunk(req, style_l);

//     char title[100];
//     make_title_html(title, sizeof(title), "NVS Dump");
//     httpd_resp_sendstr_chunk(req, (const char *) title);

//     // component-declared from app_settings_sources
//     for (uint8_t c=0; c<app_settings_sources_size; c++){
//         app_settings_source_t ass = app_settings_sources[c];
//         uint out_buff_len = 120 * ass.n_settings + 50;
//         char out_buff[out_buff_len];
//         size_t index = snprintf(out_buff, out_buff_len, "<H2>%s</H2>\n<ul>\n", ass.source_name);
//         for (int i=0; i<ass.n_settings; i++){
//             const char* key = ((const char **)ass.settings_available_ptr)[i];
//             ass.settings_get_str_fn(key, current, original);
//             index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>%s: current=%s, default=%s</li>", key, current, original); 
//         }
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "</ul>");
//         httpd_resp_sendstr_chunk(req, (const char *)out_buff);

//         if (ass.config_report != NULL) {
//             httpd_resp_sendstr_chunk(req, "<H3>Compiled Config</H3>\n");
//             httpd_resp_sendstr_chunk(req, *ass.config_report);
//         }
//     }

//     // Persistent runtime - explicit here by name and type. Naming convention is lower case for these vs upper case for settings.
//     // Coverage: self-check, app and data logging pointers, ...
//     httpd_resp_sendstr_chunk(req, "<H2>Persistent Runtime</H2>\n");
//     char out_buff[200];
//     uint out_buff_len = sizeof(out_buff);
//     float f_value;
//     uint16_t u16_value;
    
//     /*
//     >>>>>>> this section illustrative of snprintf approach. NB return value is the no of chars which WOULD HAVE been addd to the buffer had it
//     been large enough. The 2nd param in the appending snprintf() ensures max buffer length is respected.
//     The index value cannot be relied on at the end so I use httpd_resp_sendstr_chunk rather than the "_send_" version with an explicit length.
//     This approach also has the property that the compiler is able to check buffer use at compile time.
//     */ 
//     size_t index = snprintf(out_buff, out_buff_len, "<B>Self-check</B>\n<ul>\n");
//     if (pr_get_float("last_vin", &f_value) == ESP_OK) 
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>last_vin: %.2fV</li>", f_value);
//     #ifdef CONFIG_CORE_RH_MONITOR
//     if (pr_get_float("last_rh", &f_value) == ESP_OK)
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>last_rh: %.1f%%</li>", f_value);
//     #endif
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "</ul>");
//     httpd_resp_sendstr_chunk(req, (const char *)out_buff);

//     index = snprintf(out_buff, out_buff_len, "<b>Data Logger Addresses</b>\n<ul>\n");  // also on download page, but in hex here
//     uint32_t data_start;
//     uint32_t data_tail;
//     uint32_t data_head;
//     get_addresses(&data_start, &data_tail, &data_head);
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>data_start: 0x%06lx</li>", data_start);
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>data_tail: 0x%06lx</li>", data_tail);
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>data_head: 0x%06lx</li>", data_head);
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "</ul>");
//     httpd_resp_sendstr_chunk(req, (const char *)out_buff);

//     index = snprintf(out_buff, out_buff_len, "<b>Misc</b>\n<ul>\n");
//     pr_get_uint16("app_log_slot", &u16_value);
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<li>app_log_slot: 0x%03x</li>", u16_value);
//     index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "</ul>");
//     httpd_resp_sendstr_chunk(req, (const char *)out_buff);

//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);  // end chunks

//     return ESP_OK;
// }

// static esp_err_t settings_post_handler(httpd_req_t *req){
//     (*activity_callback)('R', req->uri);

//     esp_err_t err;

//     int c_len = req->content_len;
//     // ESP_LOGI(TAG, "req->conten_len = %u", c_len);
//     if (c_len == 0){  // no content!
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No form data.");
//         return ESP_FAIL;
//     }

//     char buf[c_len + 1];  // add space for null termination, which is needed for reliable extraction of parameters
//     // small payload will never be chunked
//     httpd_req_recv(req, buf, sizeof(buf));
//     buf[c_len] = 0;

//     char return_page[16] = "settings";
//     char key[16];
//     char value[SETTINGS_CO_BUFF_LEN];
//     char c[2];
//     uint8_t component;
//     // return_page allows for NVS KV setting of calibration parameters (etc) which are POSTED from different pages
//     httpd_query_key_value(buf, "return_page", return_page, sizeof(key));
//     // Get key & value of expected key - format as query-string
//     if (httpd_query_key_value(buf, "key", key, sizeof(key)) == ESP_OK) {
//         err = httpd_query_key_value(buf, "value", value, sizeof(value));
//         if ((strlen(value) > 0 ) && (err == ESP_OK)) {
//             if (httpd_query_key_value(buf, "c", c, sizeof(c)) == ESP_OK) {
//                 component = atoi(c);
//             } else {
//                 httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing component");
//                 return ESP_FAIL;    
//             }
//         } else {
//             if (err == ESP_ERR_HTTPD_RESULT_TRUNC) {
//                 httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Value was too long (exceeds SETTINGS_CO_BUFF_LEN)");
//             } else {
//                 httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing value");
//             }
//             return ESP_FAIL;
//         }
//     } else {
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing key");
//         return ESP_FAIL;
//     }

//     app_settings_source_t ass = app_settings_sources[component];
//     err = ass.settings_store_str_fn(key, value);
//     switch (err)
//     {
//         case ESP_OK:
//             break;
//         case ESP_ERR_INVALID_ARG:
//             httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unsupported key parameter");
//             return ESP_FAIL;
//         default:
//             httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
//             return ESP_FAIL;
//     }

//     // read from NVS for hopefully-confirmatory msg
//     char o[SETTINGS_CO_BUFF_LEN];
//     ass.settings_get_str_fn(key, value, o);
//     char resp_buff[200];
//     snprintf(resp_buff, sizeof(resp_buff), "Set %s to %s (default %s). <a href=\"/%s?c=%d\">Return</a>.", key, value, o, return_page, component);
    
//     httpd_resp_sendstr_chunk(req, style_l);
//     httpd_resp_sendstr_chunk(req, resp_buff);
//     httpd_resp_send_chunk(req, NULL, 0);  // end chunks
    

//     return ESP_OK;
// }

// static const httpd_uri_t settings_get = {
//     .uri       = "/settings",
//     .method    = HTTP_GET,
//     .handler   = settings_get_handler
// };

// static const httpd_uri_t nvs_dump_get = {
//     .uri       = "/nvs_dump",
//     .method    = HTTP_GET,
//     .handler   = nvs_dump_get_handler
// };

// static const httpd_uri_t settings_post = {
//     .uri       = "/settings",
//     .method    = HTTP_POST,
//     .handler   = settings_post_handler
// };

// static esp_err_t core_calibration_get_handler(httpd_req_t *req){
//     (*activity_callback)('R', req->uri);

//     char out_buff[350];
//     uint out_buff_len = sizeof(out_buff);

//     httpd_resp_sendstr_chunk(req, style_l);
    
//     snprintf(out_buff, out_buff_len, "<H1>Core Calibration</H1>\n<H2>Battery/USB Voltage - 'Vin'</H2>");
//     httpd_resp_sendstr_chunk(req, (const char *)out_buff);
//     // take reading
//     float v_in;
//     selfcheck_vin(&v_in, out_buff, sizeof(out_buff), false);
//     httpd_resp_send_chunk(req, (const char *)out_buff, HTTPD_RESP_USE_STRLEN);
//     // form to change scale factor
//     extern float adc_vin_scale;    
//     snprintf(out_buff, out_buff_len, core_calib_form_l, app_settings_ix_lookup("CORE"), adc_vin_scale);
//     httpd_resp_sendstr_chunk(req, (const char *)out_buff);
    
//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);  // end chunks

//     return ESP_OK;
// }

// static const httpd_uri_t core_calibration_get = {
//     .uri       = "/core_calib",
//     .method    = HTTP_GET,
//     .handler   = core_calibration_get_handler
// };


// // Sensor - currently limited to live reading.
// static esp_err_t sensor_get_handler(httpd_req_t *req){
//     (*activity_callback)('R', req->uri);

//     char out_buff[350];
//     uint out_buff_len = sizeof(out_buff);
//     char line_buff[100];

//     httpd_resp_sendstr_chunk(req, style_l);
        
//     // if (!live_reading_callback(line_buff, sizeof(line_buff), true) == ESP_OK) {
//         // strcpy(line_buff, "<p><b>Read Failed</b></p>");
//     // }
//     live_reading_callback(line_buff, sizeof(line_buff), true);
//     snprintf(out_buff, out_buff_len, "<H1>Sensor</H1>\n<H2>Live Reading</H2>\n<p><i>Refresh page to update</i></p>\n%s\n", line_buff);
//     httpd_resp_sendstr_chunk(req, (const char *)out_buff);
    
//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);  // end chunks

//     return ESP_OK;
// }

// static const httpd_uri_t sensor_get = {
//     .uri       = "/sensor",
//     .method    = HTTP_GET,
//     .handler   = sensor_get_handler
// };


// static esp_err_t timestamp_post_handler(httpd_req_t *req)
// {
//     int c_len = req->content_len;
//     // ESP_LOGI(TAG, "req->conten_len = %u", c_len);
//     if (c_len == 0){  // no content!
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No form data.");
//         return ESP_FAIL;
//     }

//     char buf[c_len + 1];  // add space for null termination for echo back to client (temporary)
//     // small payload will never be chunked
//     httpd_req_recv(req, buf, sizeof(buf));
//     buf[c_len] = 0;

//     char param[11];
//     // Get value of expected key - format as query-string
//     if (httpd_query_key_value(buf, "ts", param, sizeof(param)) != ESP_OK) {
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'ts' key");
//     }
//     time_t ts = atoll(param);
//     if (ts > 0) {
//         ESP_LOGI(TAG, "Setting timestamp to: %lli", ts);
//         struct timeval tv = { .tv_sec = ts};
//         settimeofday(&tv, NULL);
//     } else {
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid sent value for 'ts'.");
//     }

//     // httpd_resp_sendstr_chunk(req, buf);

//     // httpd_resp_sendstr_chunk(req, menu_l);
//     // httpd_resp_send_chunk(req, NULL, 0);  // end chunks

//     // return ESP_OK;

//     return index_get_handler(req);
// }

// static const httpd_uri_t ts_post_uri = {
//     .uri       = "/api/timestamp",
//     .method    = HTTP_POST,
//     .handler   = timestamp_post_handler,
//     .user_ctx  = NULL
// };


// static esp_err_t download_get_handler(httpd_req_t *req){  // download form + current address info.
//     (*activity_callback)('R', req->uri);

//     char out_buff[350];
//     uint out_buff_len = sizeof(out_buff);

//     httpd_resp_sendstr_chunk(req, style_l);
    
//     uint32_t data_start;
//     uint32_t data_tail;
//     uint32_t data_head;
//     esp_err_t err = get_addresses(&data_start, &data_tail, &data_head);
//     uint8_t data_unit_size = get_data_unit_size();
//     uint8_t data_slot_size = get_data_slot_size();

//     size_t index = make_title_html(out_buff, out_buff_len, "Download");

//     // First check for a reset. A normal page load does not have a query string but does have a RESET link which reloads the page with ?reset=0.
//     // If that is found then a confirm link is shown, which reloads the page with ?reset=1, and a cancel link which reloads with no querystring.
//     // If the handler sees reset=1 then the reset is performed and the normal (no querystring) page is shown
//     size_t q_len = httpd_req_get_url_query_len(req) + 1;
//     if (q_len > 0){
//         char buf[q_len];
//         if (httpd_req_get_url_query_str(req, buf, q_len) == ESP_OK) {
//             // ESP_LOGI(TAG, "%s", buf);
//             if (strcmp(buf, "reset=0") == 0){
//                 index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0,
//                                     "<p>Pending reset: <a href=\"/download?reset=1\">CONFIRM</a> or <a href=\"/download\">CANCEL</a></p>\n");
//                 httpd_resp_send_chunk(req, (const char *)out_buff, index);  // Heading and links. Already started a chunked approach, so continue.
//                 httpd_resp_send_chunk(req, NULL, 0);
//                 return ESP_OK;

//             } else if (strcmp(buf, "reset=1") == 0){
//                 reset_addresses();
//                 get_addresses(&data_start, &data_tail, &data_head);  // don't assume the reset worked!
//             }
//         }
//     }

//     if (err == ESP_OK) {
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0,
//             "<p>Addresses:</p><ul><li class=\"compact\">start=%lu</li><li class=\"compact\">tail=%lu</li><li class=\"compact\">head=%lu</li></ul>",
//             data_start, data_tail, data_head);
//         float pc_used = 100 * ((float)(data_head - (float)data_tail)) / get_storage_size();
//         if (pc_used < 0) pc_used += 100;  // head had wrapped
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<p>%.1f%% full (tail to head) of %lu bytes. %u bytes/record.<p/>\n",
//         pc_used, get_storage_size(), data_slot_size);
//         httpd_resp_send_chunk(req, (const char *)out_buff, index);  // Heading and current addresses

//         // latest record and latest not downloaded. Allow read errors to simply give epoch time and 0 values as error indicator.
//         char rec_text[100];
//         char ts_text[32];
//         uint8_t bytes_from[data_unit_size];
//         uint32_t address = data_tail;
//         bool up_to_date = (data_tail == data_head);
//         if (!up_to_date) {
//             read_record(&address, bytes_from);  // NB address is advanced by fn
//             (*format_record_callback)(bytes_from, rec_text, sizeof(rec_text), false);  // text
//             (*time_of_record_callback)(bytes_from, ts_text, sizeof(ts_text), false);
//             index = snprintf(out_buff, out_buff_len, "<p>Download tail @ %s UTC<br/>%s</p>\n", ts_text, rec_text);
            
//             address = backtrack_address(data_head, 1);
//             uint8_t bytes_to[data_unit_size];
//             read_record(&address, bytes_to);
//             (*format_record_callback)(bytes_to, rec_text, sizeof(rec_text), false);  // text
//             (*time_of_record_callback)(bytes_to, ts_text, sizeof(ts_text), false);
//             index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<p>Last record @ %s UTC<br/>%s</p>\n", ts_text, rec_text);
        
//         } else {
//             // start buff afresh - avoid the previous out_buff being carried forwards -> output duplication.
//             index = snprintf(out_buff, out_buff_len, "<p>Downloads are up to date</p>\n");
//         }

//         httpd_resp_send_chunk(req, (const char *)out_buff, index);

//         // download form and reset link
//         if (up_to_date) {
//             httpd_resp_sendstr_chunk(req, download_form2_l);  // only "all" option
//         } else {
//             httpd_resp_sendstr_chunk(req, download_form_l);
//         }
//         httpd_resp_sendstr_chunk(req, "<p><a href=\"/download?reset=0\">RESET</a> (looses all data)</p>");

//     }  // else show nothing - an evident fail!

//     // index and end chunk signal
//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);

//     return ESP_OK;
// }


// // CSV download.
// static esp_err_t download_post_handler(httpd_req_t *req)
// {
//     int c_len = req->content_len;
//     // ESP_LOGI(TAG, "req->conten_len = %u", c_len);
//     if (c_len == 0){  // no content!
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No form data.");
//         return ESP_FAIL;
//     }

//     // small payload will never be chunked
//     char buf[c_len + 1];  // add space for null termination, which is needed for reliable extraction of parameters
//     httpd_req_recv(req, buf, sizeof(buf));
//     buf[c_len] = 0;

//     // Get value of expected key - format as query-string
//     char dl_range[8];
//     if (httpd_query_key_value(buf, "dl_range", dl_range, sizeof(dl_range)) != ESP_OK) {
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'dl_range' key");
//     }

//     uint32_t data_start;
//     uint32_t data_tail;
//     uint32_t data_head;
//     uint32_t address;  // read index
//     esp_err_t err = get_addresses(&data_start, &data_tail, &data_head);
//     uint8_t data_unit_size = get_data_unit_size();
//     bool range_latest = false;
//     if (strcmp(dl_range, "all") == 0){
//         address = data_start;
//     } else if (strcmp(dl_range, "snap") == 0) {
//         address = backtrack_address(data_head, CONFIG_DATA_LOG_SNAPSHOT_RECS);
//     } else {  // dl_range == "new"
//         // latest (undownloaded) records
//         address = data_tail;
//         range_latest = true;
//     }

//     // no downloadable data
//     if (address == data_head) return download_get_handler(req);

//     uint8_t bytes[data_unit_size];

//     // HTTP headers, including a file name which includes the datetime of the last record
//     uint32_t last_rec_addr = backtrack_address(data_head, 1);
//     read_record(&last_rec_addr, bytes);  // if an err then bytes should be as initialised and epoch date appear in filename
//     char ts_text[32];
//     (*time_of_record_callback)(bytes, ts_text, sizeof(ts_text), true);
//     char cd_hdr[100];  // must use separate buffer because set_hdr function stores a pointer to whatever is passed - ie it is mutable!
//     snprintf(cd_hdr, sizeof(cd_hdr), "attachment; filename=\"" CONFIG_APP_IDENTITY "_%s_data_%s_%s.csv\"", logger_location_code, dl_range, ts_text);
//     ESP_LOGI(TAG, "CSV: %s", cd_hdr);
//     httpd_resp_set_type(req, "text/csv");
//     httpd_resp_set_hdr(req, "Content-Disposition", cd_hdr);

//     // Generate CSV in chunks
//     char out_buff[1000];
//     uint out_buff_len = sizeof(out_buff);
//     char line_buff[100];
//     bool reached_end = (address == data_head);
//     size_t index = (*format_record_callback)(NULL, out_buff, out_buff_len, true);  // header
//     while (!reached_end){
//         err = read_record(&address, bytes);
//         if (err != ESP_OK) {
//             ESP_LOGE(TAG, "Error reading logger record. Address=%lu. Aborting CSV prep.", address);
//             break;
//         }
//         size_t line_len = (*format_record_callback)(bytes, line_buff, sizeof(line_buff), true);
//         if (index + line_len < out_buff_len){  // changed from <= otherwise we get lines ending NUL rather than \n when out_buff is exactly full.
//             index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "%s", line_buff);  // alt approach might use strcpy()
//         } else {
//             httpd_resp_send_chunk(req, (const char *)out_buff, index);
//             strcpy(out_buff, line_buff);
//             index = line_len;
//         }
//         reached_end = (address == data_head);
//     }
//     if (index > 0) httpd_resp_send_chunk(req, (const char *)out_buff, index);
//     httpd_resp_send_chunk(req, NULL, 0);

//     // update the address records
//     if (range_latest) err = setting_set_uint32("data_tail", data_head);

//     return err;
// }

// static const httpd_uri_t download_get = {
//     .uri       = "/download",
//     .method    = HTTP_GET,
//     .handler   = download_get_handler
// };

// static const httpd_uri_t download_post = {
//     .uri       = "/download",
//     .method    = HTTP_POST,
//     .handler   = download_post_handler,
//     .user_ctx  = NULL
// };



// static esp_err_t applog_get_handler(httpd_req_t *req){
//     (*activity_callback)('R', req->uri);

//     esp_err_t err = ESP_OK;

//     // test of EEPROM presence
//     if (!eeprom_attached){
//         httpd_resp_sendstr(req, "App logging EEPROM not found.");
//         return ESP_OK;
//     }

//     // query string decoding of end slot of log, displayed record-set size, and (backwards) offset in units of record-set. Initial page load accepts none
//     // allocate memory for length + 1 for null termination
//     uint16_t end_slot;  // last log pointer this points to an eeprom page
//     bool end_slot_set = false;
//     bool was_reset = false;
//     uint16_t offset = 0;
//     uint16_t record_set_size = 20;
//     size_t q_len = httpd_req_get_url_query_len(req) + 1;
//     if (q_len > 0){
//         char buf[q_len];
//         char param[32];
//         if (httpd_req_get_url_query_str(req, buf, q_len) == ESP_OK) {
//             // first check for EEPROM reset request, aftwer which we re-load the GET page
//             if (strcmp(buf, "reset=1") == 0){
//                 insert_null_msg(APP_LOG_TOP_SLOT);
//                 setting_set_uint16("app_log_slot", 0);
//                 was_reset = true;
//             } else if (httpd_query_key_value(buf, "end_slot", param, sizeof(param)) == ESP_OK) {
//                 end_slot = atoi(param);
//                 end_slot_set = true;
//             } else {
//                 httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'end_slot' key");
//                 return ESP_FAIL;
//             }
//             // // record_set_size
//             // if (httpd_query_key_value(buf, "record_set_size", param, sizeof(param)) == ESP_OK) {
//             //     record_set_size = atoi(param);
//             // }
//             // offset
//             if (httpd_query_key_value(buf, "offset", param, sizeof(param)) == ESP_OK) {
//                 offset = atoi(param);
//             }
//         }
//     }

//     char out_buff[500];  // large enough to apply format app_log_form_l
//     uint out_buff_len = sizeof(out_buff);

//     httpd_resp_sendstr_chunk(req, style_l);
    
//     if (!was_reset){
//         if (!end_slot_set) {
//             pr_get_uint16("app_log_slot", &end_slot);  // stored value is the next available slot for logging
//             end_slot = (end_slot == 0)?APP_LOG_TOP_SLOT:end_slot-1;
//         }
        
//         size_t index = snprintf(out_buff, out_buff_len, "<H1>App Logs</H1>\n<p>Last slot: %u</p>\n<ul>\n", end_slot);
//         httpd_resp_sendstr_chunk(req, out_buff);

//         // empty the log-to-eeprom ring buffer and set the lock to stop task-driven emptying
//         // app_logger_store();
//         app_log_lock_i2c = true;
        
//         // TODO maybe change this to a similar accumulate then send approach as used in csv download
//         uint16_t offset_s = offset * record_set_size;  // convert to slot pointer from set
//         uint16_t slot = (offset_s <= end_slot)?end_slot - offset_s:APP_LOG_TOP_SLOT - offset_s + end_slot;
//         ee_log_unit_t rec;
//         for (uint8_t i=0; i<record_set_size; i++){
//             err = read_app_log_msg(slot, &rec);
//             if (err != ESP_OK) {
//                 snprintf(out_buff, out_buff_len, "Read of slot %u failed. Aborting", slot);
//                 httpd_resp_sendstr_chunk(req, out_buff);
//                 break;
//             }
//             if (rec.msg_len == 0) break;
//             snprintf(out_buff, out_buff_len, "<li class=\"compact\">%04u: %s</li>\n", slot, rec.msg);
//             httpd_resp_sendstr_chunk(req, out_buff);

//             slot = (slot == 0)?APP_LOG_TOP_SLOT:slot - 1;  // wrap back
//         }
//         app_log_lock_i2c = false;  // release the lock

//         // forward/backward links. the initial end-slot is held as state so that new app log records do not mess up offset-paging
//         index = snprintf(out_buff, out_buff_len, "</ul>");
//         if (rec.msg_len != 0) {
//             index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<a href=\"/app_logs?offset=%u&end_slot=%u\">back</a> -\n", offset + 1, end_slot);
//         }
//         if (offset > 0) {
//             index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<a href=\"/app_logs?offset=%u&end_slot=%u\">forewards</a> -\n", offset - 1, end_slot);
//         }
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<a href=\"/app_logs?offset=0&end_slot=%u\">end</a> -\n<a href=\"/app_logs\">update</a>\n", end_slot);
//         index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "<br/><a href=\"/app_logs?reset=1\">RESET LOG</a>\n");
//         httpd_resp_sendstr_chunk(req, out_buff);

//         // form for csv downloads via POST handler
//         snprintf(out_buff, out_buff_len, app_log_form_l, offset, end_slot);
//         httpd_resp_sendstr_chunk(req, out_buff);
//     } else {
//         if (app_logger_clear() == ESP_OK) {
//             httpd_resp_sendstr_chunk(req, "<H1>App Logs</H1>\n<p>Empty</p>\n");
//         } else {
//             httpd_resp_sendstr_chunk(req, "<H1>App Logs</H1>\n<p>Failed. Use menu to reload page.</p>\n");
//         }
//     }

//     // index and end chunk signal
//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);

//     return ESP_OK;
// }

// // CSV download.
// static esp_err_t applog_post_handler(httpd_req_t *req)
// {
//     esp_err_t err = ESP_OK;

//     int c_len = req->content_len;
//     if (c_len == 0){  // no content!
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No form data.");
//         return ESP_FAIL;
//     }

//     // small payload will never be chunked
//     char buf[c_len + 1];  // add space for null termination, which is needed for reliable extraction of parameters
//     httpd_req_recv(req, buf, sizeof(buf));
//     buf[c_len] = 0;

//     // MAJOR drama here attempting to reset the log here by inserting the len=0 msg at the end of the EEPROM. Guru mediation errors. Prob because a "critical section"

//     char dl_range[8];
//     if (httpd_query_key_value(buf, "dl_range", dl_range, sizeof(dl_range)) != ESP_OK) {
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'dl_range' key");
//         return ESP_FAIL;
//     }

//     uint16_t slot_from = 0;  // work backwards from here
//     uint16_t recs_to_send = 0;  // max number, a len=0 record will trigger an early stop
//     if (strcmp(dl_range, "all") == 0){
//         // download all ignores end-slot in form and will work all the way around the circular buffer or end early of a len=0 marker is found
//         pr_get_uint16("app_log_slot", &slot_from);  // stored value is the next available slot for logging
//         if (slot_from == 0) {
//             slot_from = APP_LOG_TOP_SLOT;  // wrap back (or immediate aftermath of a app log reset)
//         } else {
//             slot_from--;
//         }
//         recs_to_send = APP_LOG_TOP_SLOT + 1;
//     } else if (strcmp(dl_range, "set") == 0) {
//         char param[8];
//         if (httpd_query_key_value(buf, "end_slot", param, sizeof(param)) == ESP_OK) {
//             slot_from = atoi(param);
//         } else {
//             httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'end_slot' key");
//             return ESP_FAIL;
//         }
//         uint16_t offset = 0;
//         uint16_t record_set_size = 20;
//         if (httpd_query_key_value(buf, "offset", param, sizeof(param)) == ESP_OK) {
//             offset = atoi(param);
//         }
//         uint16_t offset_s = offset * record_set_size;  // convert to slot offset
//         if (slot_from >= offset_s) {
//             slot_from = slot_from - offset_s;
//         } else {
//             slot_from = APP_LOG_TOP_SLOT - offset_s + slot_from;
//         }
//         recs_to_send = record_set_size;
//     } else {
//         httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid 'dl_range' key");
//         return ESP_FAIL;
//     }

//     ESP_LOGI(TAG, "App Log Dump: %s from=%u for %u recs (max)", dl_range, slot_from, recs_to_send);
    
//     char ts_text[16];
//     time_t now;
//     struct tm timeinfo;
//     time(&now);
//     gmtime_r(&now, &timeinfo);
//     strftime(ts_text, sizeof(ts_text), "%Y%m%dT%H%M%S", &timeinfo);  // avoid colons for use in csv file name
//     char cd_hdr[100];  // must use separate buffer because set_hdr function stores a pointer to whatever is passed - ie it is mutable!
//     snprintf(cd_hdr, sizeof(cd_hdr), "attachment; filename=\"" CONFIG_APP_IDENTITY "_%s_applog_%s_%s.csv\"", logger_location_code, dl_range, ts_text);
//     ESP_LOGI(TAG, "CSV: %s", cd_hdr);
//     httpd_resp_set_type(req, "text/csv");
//     httpd_resp_set_hdr(req, "Content-Disposition", cd_hdr);

//     // set the lock to stop task-driven emptying
//     app_log_lock_i2c = true;

//     // Generate CSV in chunks
//     char out_buff[1000];
//     uint out_buff_len = sizeof(out_buff);
//     char line_buff[300];
//     uint16_t slot = slot_from;
//     ee_log_unit_t rec;
//     size_t index = sprintf(out_buff, "slot,msg\n");  // header
//     while (recs_to_send > 0){
//         err = read_app_log_msg(slot, &rec);
//         if (err != ESP_OK) {
//             ESP_LOGE(TAG, "Error reading logger record. Address=%u. Aborting CSV prep.", slot);
//             break;
//         }
//         if (rec.msg_len == 0) {
//             // ESP_LOGI(TAG, "Early stop - 0 len msg @ slot %u", slot);
//             break;
//         }

//         size_t line_len = snprintf(line_buff, out_buff_len, "%04u,%s", slot, rec.msg);  // unlike data logger, do not include line break here, because...
//         // remove any line breaks from the original message (standard log messages contain a terminal, although this may be lost of stored msg was truncated)
//         for (char* newline_pos = strchr(line_buff, '\n'); (newline_pos = strchr(line_buff, '\n')) != NULL; *newline_pos = ' ');
//         // append, emitting first if out_buff hasnt enough space
//         if (index + line_len <= out_buff_len){
//             index += snprintf(&out_buff[index], (index < out_buff_len)?out_buff_len-index:0, "%s\n", line_buff);
//         } else {
//             httpd_resp_sendstr_chunk(req, (const char *)out_buff);
//             strcpy(out_buff, line_buff);
//             out_buff[line_len] = '\n';
//             index = line_len + 1;
//         }

//         slot = (slot > 0)?slot - 1:APP_LOG_TOP_SLOT;
//         recs_to_send--;
//     }

//     if (index > 0) httpd_resp_sendstr_chunk(req, (const char *)out_buff);
//     httpd_resp_send_chunk(req, NULL, 0);

//     app_log_lock_i2c = false;  // free the lock


//     return err;
// }

// static const httpd_uri_t applog_get = {
//     .uri       = "/app_logs",
//     .method    = HTTP_GET,
//     .handler   = applog_get_handler
// };

// static const httpd_uri_t applog_post = {
//     .uri       = "/app_logs",
//     .method    = HTTP_POST,
//     .handler   = applog_post_handler
// };

// // without query string, this presents a "confirm" link which requests the same page with a query string set
// // with a query string (ANY!), performs a SW reset
// static esp_err_t reset_handler(httpd_req_t *req)
// {
//     if (httpd_req_get_url_query_len(req) > 0) {
//         ESP_LOGI(TAG, "Webserver triggered restart");
//         vTaskDelay(2);
//         app_logger_store();
//         esp_restart();
//     }

//     (*activity_callback)('R', req->uri);

//     httpd_resp_sendstr_chunk(req, style_l);
//     httpd_resp_sendstr_chunk(req, "<H1>Restart Device</H1>\n<a href=\"/reset?confirm=1\">CONFIRM RESET</a>\n<p>Give it a while...</p>");
//     httpd_resp_sendstr_chunk(req, menu_l);
//     httpd_resp_send_chunk(req, NULL, 0);

//     return ESP_OK;
// }

// static const httpd_uri_t reset = {
//     .uri       = "/reset",
//     .method    = HTTP_GET,
//     .handler   = reset_handler
// };

// static esp_err_t quit_handler(httpd_req_t *req)
// {
//     time_t now;
//     time(&now);
//     if (now < 1750594779) {
//         (*activity_callback)('R', req->uri);
//         httpd_resp_sendstr(req, "Quit is blocked; the timestamp is not set.");
//     } else {
//         (*activity_callback)('Q', req->uri);
//         httpd_resp_sendstr(req, "Quitting webserver; device will go to sleep.");
//     }
    
//     return ESP_OK;
// }

// static const httpd_uri_t quit = {
//     .uri       = "/quit",
//     .method    = HTTP_GET,
//     .handler   = quit_handler
// };

// static esp_err_t sleep_handler(httpd_req_t *req)
// {
//     (*activity_callback)('S', req->uri);
//     httpd_resp_sendstr(req, "Exit to sleep without timer wake. Button wake only.");
     
//     return ESP_OK;
// }

// static const httpd_uri_t sleep = {
//     .uri       = "/sleep",
//     .method    = HTTP_GET,
//     .handler   = sleep_handler
// };

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

    config.stack_size = 6000;

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
