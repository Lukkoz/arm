# Design notes

## PCA9685 servo driver

Files: `pca9685.h/.cpp` (I2C driver), `servo_output.h/.cpp` (joint angle → pulse), settings in `pinout.h`.

### How the chip makes a pulse
The PCA9685 has 16 outputs and one shared 12-bit counter.

- The counter runs from **0 to 4095** and then starts over. One full count is one PWM period.
- Each channel has two numbers:
  - **ON**: the count where the output goes **high**.
  - **OFF**: the count where it goes **low**.
- The pulse width is the time between them, (OFF − ON) counts.

```
counter: 0 ─────────────────────────────────────────── 4095 │ 0 ──
output:  ▔▔▔▔▔▔▔▔▔╲_________________________________________│▔▔▔▔
         ON=0    OFF=307                                    next period
         |<-1.5 ms->|
```

### Setting the 50 Hz period
Servos expect one pulse every 20 ms (50 Hz). The chip divides its 25 MHz oscillator with a prescaler:

```
prescale = round(25 000 000 / (4096 × 50)) − 1 = 121
1 count  = (121 + 1) / 25 MHz ≈ 4.88 µs
period   = 4096 × 4.88 µs ≈ 20.0 ms
```

The chip only accepts a new prescaler while it is asleep. That's why `Pca9685::begin()` puts it to sleep, writes the value, wakes it and restarts it.

### Converting the servo settings
The Studio servo settings are in microseconds and degrees (min 500 µs, max 2500 µs, range 180°). The driver converts them to counts:

| Servo angle | Pulse | OFF count (ON = 0) |
|---|---|---|
| 0° | 500 µs | 102 |
| 90° (joint 0 on the ARM) | 1500 µs | 307 |
| 180° | 2500 µs | 512 |

So a servo uses only counts 102–512 of the 4096. That's about 410 steps over 180°, roughly **0.44° per step**: fine for hobby servos, which can't position more precisely than that anyway.

In the code:
1. `servo_output.cpp` turns the joint angle into a servo angle (zero offset + direction × joint angle, limited to the servo range).
2. It then turns that angle into a pulse: `min + angle / range × (max − min)`.
3. `Pca9685::writeMicroseconds()` divides the pulse by 4.88 µs to get counts.
4. `Pca9685::setPwm(channel, 0, counts)` writes ON = 0 and OFF = counts. These are four bytes per channel (`ON_L, ON_H, OFF_L, OFF_H`), starting at register `0x06 + 4 × channel`.

### Special values
- **OFF = 4096** sets bit 12, which means "always off". `off()` and `allOff()` use it: there are no pulses at all, so the servos go limp. That's the state at power-up, until the first move.
- **ON = 4096** would mean "always on". It's never used for servos.

### Oscillator accuracy
The "25 MHz" oscillator is really anywhere from about 25 to 27 MHz depending on the board. If yours runs fast, every pulse comes out a few percent too long, so the servos sit a few degrees off and travel slightly more than they should. To correct it:
1. Measure the PWM frequency on one channel with a scope or logic analyzer.
2. Adjust `PCA9685_OSC_HZ` in `pinout.h` until the frequency reads 50 Hz.

### Note: the ON value is always 0 (open decision)
`setPwm(channel, on, off)` matches the chip's registers, but every caller passes ON = 0:
- `writeMicroseconds()` uses `setPwm(channel, 0, ticks)`.
- `off()` uses `setPwm(channel, 0, 4096)`; nothing in the firmware calls `off()` at the moment.

So right now the `on` parameter isn't used for anything. It's private to the driver, so leaving it as is causes no harm, but it should be resolved one way or the other:

1. **Stagger the channels (recommended).** Give each channel its own start point. The pulse width and frequency stay exactly the same; only the moment within the 20 ms frame changes.
   ```cpp
   uint16_t on  = channel * 256;          // channel 0 starts at count 0, channel 6 at 1536, ...
   uint16_t off = (on + ticks) & 0x0FFF;  // the chip handles OFF < ON by wrapping past 4095
   setPwm(channel, on, off);
   ```
   **Why it helps:** with ON = 0 all servos go high at the same moment every frame, so their current spikes add up on the servo supply. Staggered starts spread the spikes over the frame. That reduces voltage dips, jitter and brown-out resets, which matters most when the servos share a weak supply with the controller.
2. **Simplify.** Remove the parameter, use `setPwm(channel, off)` and always write ON = 0. The code says exactly what it does, but you lose the benefit above.

Status: not changed yet. Decide together with the open firmware issues.
