// This is the "unit test" example from the Zorxx github repo: https://github.com/zorxx/neopixel/tree/main
// SUPPLEMENTED with a basic sine wave prototype for the Neopixel Wave firmware
/* \copyright 2023 Zorxx Software. All rights reserved.
 * \license This file is released under the MIT License. See the LICENSE file for details.
 * \brief ESP32 Neopixel Driver Library Example Application
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "neopixel.h"

#include <math.h>

#define TAG "n-wave"
#define PIXEL_COUNT 100
#define NEOPIXEL_PIN GPIO_NUM_23

#if !defined(MAX)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// There are three channels with independed wave specifications which are mapped to either RGB or HSL
// Modulation of amplitude and wavelength uses a triangle form, with t=0 having no modulation and the amp/lambda being maximally REDUCED at the triangle peak
typedef struct sChannelDef
{
    char waveform;             // 'c' for cosine, 't' for triangle, 's' for square, 'u' for uniform (constant - only amplitude properties apply)
    float ratio;                // applies to triangle and square waves. Gives the fraction along the wavelenth of the max for triangle, or is the mark:space ratio for square
    float lambda;              // wavelength in number of pixels
    float lambda_mod_depth;    // fraction of wavelength REDUCTION at maximum modulation. 0 for no modulation. 0.9 will give 10% at max
    float lambda_mod_ratio;    // position of triangle peak. 0=ramp down, 0.5=symmetric triangle, 1=ramp up
    float lambda_mod_period_s; // period for modulation
    float amplitude;           // amplitude in range 0.0-1.0
    float amp_mod_depth;       // similar to lambda
    float amp_mod_ratio;
    float amp_mod_period_s;
    float velocity;            // speed of the wave in pixels/s. Negative is towards the ESP32 and positive is away.
    float phase;               // value in range -1.0 to 1.0 for fraction of wavelength by which the wave is shifted. Negative towards ESP32, positive away.
    // TODO add velocity modulation ???

} tChannelDef;

// Used internally with post-modulation lambda and amp
typedef struct sChannelDef2
{
    char waveform;             // 'c' for cosine, 't' for triangle, 's' for square, 'u' for uniform (constant - only amplitude properties apply)
    float ratio;                // applies to triangle and square waves. Gives the fraction along the wavelenth of the max for triangle, or is the mark:space ratio for square
    float lambda;              // wavelength in number of pixels
    float amplitude;           // amplitude in range 0.0-1.0
    float velocity;            // speed of the wave in pixels/s. Negative is towards the ESP32 and positive is away.
    float phase;               // value in range -1.0 to 1.0 for fraction of wavelength by which the wave is shifted. Negative towards ESP32, positive away.

} tChannelDef2;

// generate a channel value for a pixel, which will either be scaled to an RGB byte or be a HSL value which is then mapped to RGB.
// NB return values in range 0.0-1.0
static float channel_value(tChannelDef2 * c, uint32_t t_ms, uint32_t pixel_index){
    float retval = 0;
    float x_prime = fmod(((float)pixel_index - c->velocity * t_ms / 1000.0) / c->lambda + c->phase, 1.0);
    if (x_prime < 0)
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

inline static float modulate_amp(float amplitude, float amp_mod_depth, float amp_mod_ratio, float amp_mod_period_s, uint32_t t_ms){
    // amplitude of output may be modulated. A triangle is assumed
    float a_scaled = amplitude;
    if (amp_mod_depth > 0)
    {
        float a = fmod(0.001 * t_ms / amp_mod_period_s, 1.0);
        if (a <= amp_mod_ratio)
        {
            a_scaled *= 1.0 + amp_mod_depth * (a / amp_mod_ratio - 1.0);
        }
        else
        {
            a_scaled *= 1.0 + amp_mod_depth * (amp_mod_ratio - a) / (1.0 - amp_mod_ratio);
        }
    }
    return a_scaled;
}

static bool test_wave(uint32_t duration_s, tChannelDef * c1, tChannelDef * c2)
{
    tNeopixelContext neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN, true);

    if (NULL == neopixel)
    {
        ESP_LOGE(TAG, "[%s] Initialization failed\n", __func__);
        return false;
    }

    TickType_t xLastWakeTime;

    // the update period, specified in ms but then rounded to that for an integer number of ticks
    uint32_t TIME_INTERVAL_MS = 100; // min value should be 10 for standard tick period.
    uint32_t time_interval_ticks = MAX(1, pdMS_TO_TICKS(TIME_INTERVAL_MS));
    uint32_t time_interval_ms = pdTICKS_TO_MS(time_interval_ticks); // dt

    uint32_t iterations = duration_s * 1000 / time_interval_ms;

    ESP_LOGI(TAG, "[%s] Starting (time_interval=%ums, v=%.1f, lambda=%.1f)", __func__, time_interval_ms, c1->velocity, c1->lambda);

    uint32_t t_ms = 0; // time
    xLastWakeTime = xTaskGetTickCount();

    tNeopixel pixel[PIXEL_COUNT];
    for (int i = 0; i < iterations; ++i)
    {
        // // amplitude of output may be modulated. A triangle is assumed
        // float a_scale = c1->amplitude;
        // if (c1->amp_mod_depth > 0)
        // {
        //     float a = fmod(0.001 * t_ms / c1->amp_mod_period_s, 1.0);
        //     if (a <= 0.5)
        //     {
        //         a_scale *= 1.0 + c1->amp_mod_depth * (a / c1->amp_mod_ratio - 1.0);
        //     }
        //     else
        //     {
        //         a_scale *= 1.0 + c1->amp_mod_depth * (1.0 - a / c1->amp_mod_ratio);
        //     }
        // }

        tChannelDef2 c1_ = {
            .waveform = c1->waveform,
            .ratio = c1->ratio,
            .amplitude = modulate_amp(c1->amplitude, c1->amp_mod_depth, c1->amp_mod_ratio, c1->amp_mod_period_s, t_ms),
            .lambda = c1->lambda,
            .velocity = c1->velocity,
            .phase = c1->phase
        };
        tChannelDef2 c2_ = {
            .waveform = c2->waveform,
            .ratio = c2->ratio,
            .amplitude = modulate_amp(c2->amplitude, c2->amp_mod_depth, c2->amp_mod_ratio, c2->amp_mod_period_s, t_ms),
            .lambda = c2->lambda,
            .velocity = c2->velocity,
            .phase = c2->phase
        };

        for (int p = 0; p < PIXEL_COUNT; p++)
        {
            float a_1 = channel_value(&c1_, t_ms, p);
            float a_2 = channel_value(&c2_, t_ms, p);
        
            uint8_t a_1_data = a_1 * 255; // a should be in range 0.0-1.0 but RGB pixel components in range 0-255.
            uint8_t a_2_data = a_2 * 255;

            pixel[p].index = p;
            pixel[p].rgb = NP_RGB(a_1_data, a_2_data, 0);
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
    for (;;)
    {
        // test_wave(uint32_t duration_s, uint32_t lambda, float v, float amplitude, float amp_mod_depth, float amp_mod_period_s)
        tChannelDef channel1 = {
            .waveform = 's',
            .ratio = 0.5,
            .lambda = 10,
            .velocity = 10,
            .amplitude = 0.25,
            .amp_mod_depth=0.95,
            .amp_mod_ratio=0.2,
            .amp_mod_period_s=10
        };
        tChannelDef channel2 = {  // or I could have done channel2 = channel1; channel2.phase=0.1 (shallow copy OK since there are no pointers in the struct)
            .waveform = 's',
            .ratio = 0.5,
            .lambda = 10,
            .velocity = 10,
            .phase = 0.1,
            .amplitude = 0.25,
            .amp_mod_depth=0.95,
            .amp_mod_ratio=0.2,
            .amp_mod_period_s=10
        };
        test_wave(20, &channel1, &channel2);
        // test_wave(20, 20, -10.0, 0.25, 0.95, 10.0);
    }
}