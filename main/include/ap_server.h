/* WiFi AP and HTTP Server (HTTP and API).

See sdkconfig.defaults

IP address expected to be 192.168.4.1 - but see logging

No authentication or encryption as this would only be started by pressing a button on the unit.
*/

#include "http_server.h"
#include <time.h>

// #ifndef AP_SERVER_H
extern time_t ap_last_event_time;
// A=started AP, C=client connected, D=client disconnected, N=not (or de-) initialised, Q=quit requested, W=webserver started, R=webserver request
extern char ap_last_event_code;

extern char logger_location_code[16];

void begin_ap_server();
void end_ap_server();
// #define AP_SERVER_H
// #endif
