#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "neopixel.h"

#include "struct_defs.h"

#include "ap_server.h"

#include <math.h>

#define TAG "n-wave"
#define PIXEL_COUNT 100
#define NEOPIXEL_PIN GPIO_NUM_23

#if !defined(MAX)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Storage for the current wave.
char channel_map = 'r';
// These should be initialised to 0/null elements.
tChannelDef c1 = {};
tChannelDef c2 = {};
tChannelDef c3 = {};


// generate a channel value for a pixel, which will either be scaled to an RGB byte or be a HSL value which is then mapped to RGB.
// NB return values in range 0.0-1.0
static float channel_value(tChannelDef2 * c, uint32_t t_ms, uint32_t pixel_index){
    float retval = 0;
    float x_prime = fmod(((float)pixel_index - c->velocity * t_ms / 1000.0) / c->lambda + c->phase, 1.0);
    while (x_prime < 0)
        x_prime += 1.0;

    if (c->waveform == 'c') {
        // cosine
        retval = c->amplitude * 0.5 * (cosf(6.28318 * x_prime) + 1);
    } else if (c->waveform == 't') {
        // triangle
        if (x_prime <= c->ratio){
            retval = c->amplitude * x_prime / c->ratio;
        } else {
            retval = c->amplitude * (1.0 - x_prime) / (1.0 - c->ratio);
        }
    } else if (c->waveform == 's') {
        // "square"
        if (x_prime <= c->ratio)
        {
            retval = c->amplitude;
        }
        // else
        // {
        //     a_1 = 0;
        // }
    } else if (c->waveform == 'u') {
        // uniform
        retval = c->amplitude;
    }

    if (retval < 0) {
        retval = 0;
    } else if (retval > 1.0){
        retval = 1.0;
    }

    return retval;
}

inline static float modulate(float baseline, float mod_depth, float mod_ratio, float mod_period_s, float mod_offset_s, uint32_t t_ms){
    // velocity or amplitude of output may be modulated. A triangle is assumed
    float scaled = baseline;
    if (mod_depth > 0)
    {
        float a = fmod((0.001 * t_ms + mod_offset_s) / mod_period_s, 1.0);
        if (a <= mod_ratio)
        {
            scaled *= 1.0 + mod_depth * (a / mod_ratio - 1.0);
        }
        else
        {
            scaled *= 1.0 + mod_depth * (mod_ratio - a) / (1.0 - mod_ratio);
        }
        // printf("a=%.3f scaled=%.1f\n", a, scaled);
    }
    return fmax(0.0, scaled);
}

static bool play_wave(uint32_t duration_s)
{
    tNeopixelContext neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN, true);

    if (NULL == neopixel)
    {
        ESP_LOGE(TAG, "[%s] Initialization failed\n", __func__);
        return false;
    }

    TickType_t xLastWakeTime;

    // min value of 5 combined with 500 in TIME_INTERVAL_MS gives a max update time of 100ms
    float max_velocity = fmax(5.0, fmax(fabs(c1.velocity), fmax(fabs(c2.velocity), fabs(c3.velocity))));

    // the update period, specified in ms but then rounded to that for an integer number of ticks
    uint32_t TIME_INTERVAL_MS = MAX((uint32_t)(500 / max_velocity) , 10);  // 100; // min value should be 10 for standard tick period.
    uint32_t time_interval_ticks = MAX(1, pdMS_TO_TICKS(TIME_INTERVAL_MS));
    uint32_t time_interval_ms = pdTICKS_TO_MS(time_interval_ticks); // dt

    uint32_t iterations = duration_s * 1000 / time_interval_ms;

    ESP_LOGI(TAG, "[%s] Starting (time_interval=%ums, v=%.1f, lambda=%.1f)", __func__, time_interval_ms, c1.velocity, c1.lambda);

    uint32_t t_ms = 0; // time
    xLastWakeTime = xTaskGetTickCount();

    tNeopixel pixel[PIXEL_COUNT];
    for (int i = 0; i < iterations; ++i)
    {

        tChannelDef2 c1_ = {
            .waveform = c1.waveform,
            .ratio = c1.ratio,
            .amplitude = modulate(c1.amplitude, c1.amp_mod_depth, c1.amp_mod_ratio, c1.amp_mod_period_s, c1.amp_mod_offset_s, t_ms),
            .lambda = c1.lambda,
            .velocity = modulate(c1.velocity, c1.velocity_mod_depth, c1.velocity_mod_ratio, c1.velocity_mod_period_s, c1.velocity_mod_offset_s, t_ms),
            .phase = c1.phase
        };
        tChannelDef2 c2_ = {
            .waveform = c2.waveform,
            .ratio = c2.ratio,
            .amplitude = modulate(c2.amplitude, c2.amp_mod_depth, c2.amp_mod_ratio, c2.amp_mod_period_s, c2.amp_mod_offset_s, t_ms),
            .lambda = c2.lambda,
            .velocity = modulate(c2.velocity, c2.velocity_mod_depth, c2.velocity_mod_ratio, c2.velocity_mod_period_s, c2.velocity_mod_offset_s, t_ms),
            .phase = c2.phase
        };
        tChannelDef2 c3_ = {
            .waveform = c3.waveform,
            .ratio = c3.ratio,
            .amplitude = modulate(c3.amplitude, c3.amp_mod_depth, c3.amp_mod_ratio, c3.amp_mod_period_s, c3.amp_mod_offset_s, t_ms),
            .lambda = c3.lambda,
            .velocity = modulate(c3.velocity, c3.velocity_mod_depth, c3.velocity_mod_ratio, c3.velocity_mod_period_s, c3.velocity_mod_offset_s, t_ms),
            .phase = c3.phase
        };

        for (int p = 0; p < PIXEL_COUNT; p++)
        {
            uint8_t r, g, b;

            float a_1 = channel_value(&c1_, t_ms, p);
            float a_2 = channel_value(&c2_, t_ms, p);
            float a_3 = channel_value(&c3_, t_ms, p);

            if (channel_map == 'h'){
                // HSL - see https://www.rapidtables.com/convert/color/hsl-to-rgb.html
                float C = (1.0 - fabs(2.0 * a_3 - 1)) * a_2;
                float X = C * (1.0 - fabs(fmod(a_1 / 0.16666, 2.0) - 1.0));
                float m = a_3 - C / 2.0;
                if (a_1 <= 0.166666){
                    r = 255 * (C + m);
                    g = 255 * (X + m);
                    b = 255 * m;
                } else if (a_1 <= 0.333333) {
                    r = 255 * (X + m);
                    g = 255 * (C + m);
                    b = 255 * m;
                } else if (a_1 <= 0.5){
                    r = 255 * m;
                    g = 255 * (C + m);
                    b = 255 * (X + m);
                } else if (a_1 <= 0.666666){
                    r = 255 * m;
                    g = 255 * (X + m);
                    b = 255 * (C + m);
                } else if (a_1 <= 0.833333){ 
                    r = 255 * (X + m);
                    g = 255 * m;
                    b = 255 * (C + m);
                } else {
                    r = 255 * (C + m);
                    g = 255 * m;
                    b = 255 * (X + m);
                }
            } else {
                // RGB if 'r' or undefined value                
                r = a_1 * 255; // a should be in range 0.0-1.0 but RGB pixel components in range 0-255.
                g = a_2 * 255;
                b = a_3 * 255;
            }
        
            pixel[p].index = p;
            pixel[p].rgb = NP_RGB(r, g, b);
        }

        neopixel_SetPixel(neopixel, pixel, ARRAY_SIZE(pixel));

        t_ms += time_interval_ms; // wrap-around at 2**32 will make a glitch, but VERY long interval between so dont care

        // could check return value to see if the task was actually delayed - it should be or we choose a too small time interval
        xTaskDelayUntil(&xLastWakeTime, time_interval_ticks);
    }

    // for (int p = 0; p < PIXEL_COUNT; p++)
    // {
    //     printf("{%lu}: R={%lu}\n", pixel[p].index, NP_RGB2RED(pixel[p].rgb));
    // }

    ESP_LOGI(TAG, "[%s] Finished", __func__);
    neopixel_Deinit(neopixel);
    return true;
}

void app_main(void)
{
    begin_ap_server();

    // simulate a load from Flash
    tWaveDef wave = {
        .channel_map = 'h',
        .channel1 = {  // hue
            .waveform = 'c',
            .lambda = 20,
            .velocity = 4,
            .amplitude = 1.0,
            .amp_mod_depth=0.8,
            .amp_mod_period_s = 20,
            .amp_mod_ratio=0.5
        },
        .channel2 = {  // saturation
            .waveform = 'u',
            .amplitude = 1.0
        },
        .channel3 = {  // luminance
            .waveform = 'u',
            .amplitude = 0.25}
    };

    channel_map = wave.channel_map;

    c1 = wave.channel1;
    c2 = wave.channel2;
    c3 = wave.channel3;

    for (;;)
    {
        play_wave(120);
    }
}