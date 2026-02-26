#include <Arduino.h>

#define NUM_CHANNELS 6
#ifdef ARDUINO_ARCH_ESP321
//                         CH0   CH1   CH2   CH3   CH4   CH5
static const uint8_t  CHANNEL_PINS[NUM_CHANNELS]    = { 2,    4,    5,   12,   13,   14  };
static const uint16_t CHANNEL_MIN [NUM_CHANNELS]    = { 0,    0,    0,    0,    0,    0  };
static const uint16_t CHANNEL_MAX [NUM_CHANNELS]    = { 1023, 1023, 1023, 1023, 1023, 1023 };

// ─────────────────────────────────────────────────────────────────────────────
// Configuration B – alternate board layout
// ─────────────────────────────────────────────────────────────────────────────
#else 

//                         CH0   CH1   CH2   CH3   CH4   CH5
static const uint8_t  CHANNEL_PINS[NUM_CHANNELS]    = { 3,   5,   6,   9,   10,   11  };
static const uint16_t CHANNEL_MIN [NUM_CHANNELS]    = { 50,   50,    50,    50,  50,    50  };
static const uint16_t CHANNEL_MAX [NUM_CHANNELS]    = { 255,  255, 255,  255,  255, 255 };

#endif


// ─────────────────────────────────────────────────────────────────────────────
// ESP32 PWM  (LEDC peripheral)
// ─────────────────────────────────────────────────────────────────────────────
#ifdef ARDUINO_ARCH_ESP321
#include "driver/ledc.h"

// LEDC resolution used for all channels (10-bit → 0-1023)
#define LEDC_RESOLUTION  LEDC_TIMER_10_BIT
#define LEDC_FREQ_HZ     5000
#define LEDC_SPEED_MODE  LEDC_LOW_SPEED_MODE

/**
 * Initialise LEDC timer + channels for all defined pins.
 * Call once in setup().
 */
static inline void pwm_init_all()
{
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode       = LEDC_SPEED_MODE;
    timer_cfg.timer_num        = LEDC_TIMER_0;
    timer_cfg.duty_resolution  = LEDC_RESOLUTION;
    timer_cfg.freq_hz          = LEDC_FREQ_HZ;
    timer_cfg.clk_cfg          = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_cfg);

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
    {
        ledc_channel_config_t ch_cfg = {};
        ch_cfg.speed_mode = LEDC_SPEED_MODE;
        ch_cfg.channel    = static_cast<ledc_channel_t>(ch);
        ch_cfg.timer_sel  = LEDC_TIMER_0;
        ch_cfg.intr_type  = LEDC_INTR_DISABLE;
        ch_cfg.gpio_num   = CHANNEL_PINS[ch];
        ch_cfg.duty       = 0;
        ch_cfg.hpoint     = 0;
        ledc_channel_config(&ch_cfg);
    }
}

/**
 * Set PWM on a channel.
 *
 * @param channel  Channel index (CH0 … CH5)
 * @param value    User value in [0, 180]. Automatically clamped, then mapped
 *                 to the channel's [MIN, MAX] hardware window (0-1023) before
 *                 being written to the LEDC peripheral.
 */
static inline void pwm_set(uint8_t channel, uint16_t value)
{
    if (channel >= NUM_CHANNELS) return;

    const uint16_t lo       = CHANNEL_MIN[channel];
    const uint16_t hi       = CHANNEL_MAX[channel];
    const uint16_t user_max = 180u;
    const uint32_t hw_max   = 1023u;

    // Clamp user input to [0, 180]
    if (value > user_max) value = user_max;

    // Map [0, 180] → [0, 1023] full hardware range, then clamp to [MIN, MAX]
    uint16_t duty = lo + value/user_max * (hi - lo);

    ledc_set_duty  (LEDC_SPEED_MODE, static_cast<ledc_channel_t>(channel), duty);
    ledc_update_duty(LEDC_SPEED_MODE, static_cast<ledc_channel_t>(channel));
}

#else // PLATFORM_ESP32


static inline void pwm_init_all()
{
    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        pinMode(CHANNEL_PINS[ch], OUTPUT);
}

/**
 * Set PWM on a channel.
 *
 * @param channel  Channel index (CH0 … CH5)
 * @param value    User value in [0, 180]. Automatically clamped, then mapped
 *                 to the channel's [MIN, MAX] hardware window (0-255) before
 *                 being written via analogWrite.
 */
static inline void pwm_set(uint8_t channel, uint16_t value)
{
    if (channel >= NUM_CHANNELS) return;

    const uint16_t user_max = 180u;
    const uint16_t hw_max   = 255u;

    // Clamp user input to [0, 180]
    if (value > user_max) value = user_max;

    const uint16_t lo = CHANNEL_MIN[channel];
    const uint16_t hi = CHANNEL_MAX[channel];

    // Map [0, 180] → [0, 255] full hardware range, then clamp to [MIN, MAX]
    uint16_t duty = lo + value/user_max * (hi - lo);

    analogWrite(CHANNEL_PINS[channel], (int)duty);
}

#endif  // PLATFORM_ATMEGA