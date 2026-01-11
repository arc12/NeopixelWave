// #ifndef HTTP_SERVER_H
esp_err_t start_webserver(void (*activity_callback)(char, const char*));
esp_err_t stop_webserver(void);

// #define HTTP_SERVER_H
// #endif