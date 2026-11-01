#ifndef CONTRA_APU_H
#define CONTRA_APU_H

#include <stddef.h>
#include <stdint.h>

/*
 * A 2A03 APU model (NTSC): two pulse channels, triangle, noise and the DMC,
 * the frame sequencer, and the NES's non-linear mixer. The sound engine
 * writes its registers ($4000-$4017) exactly as on hardware; contra_apu_run
 * then clocks the chip for a stretch of CPU cycles and resamples the output
 * to CONTRA_AUDIO_SAMPLE_RATE (mono, signed 16-bit).
 */

#define CONTRA_AUDIO_SAMPLE_RATE 48000
#define CONTRA_AUDIO_MAX_FRAME_SAMPLES 1024

typedef uint8_t (*ContraApuMemRead)(void *ctx, uint16_t addr); /* DMC sample fetches */

typedef struct ContraApuEnvelope
{
    uint8_t start, divider, decay, period, loop, constant;
} ContraApuEnvelope;

typedef struct ContraApuPulse
{
    uint8_t enabled, duty, step, length;
    uint16_t period, timer;
    ContraApuEnvelope env;
    uint8_t sweep_enabled, sweep_period, sweep_negate, sweep_shift, sweep_reload, sweep_divider;
    uint8_t ones_complement; /* pulse 1 negates with one's complement */
} ContraApuPulse;

typedef struct ContraApuTriangle
{
    uint8_t enabled, control, linear_reload_value, linear_counter, linear_reload, step, length;
    uint16_t period, timer;
} ContraApuTriangle;

typedef struct ContraApuNoise
{
    uint8_t enabled, mode, length;
    uint16_t period, timer, shift;
    ContraApuEnvelope env;
} ContraApuNoise;

typedef struct ContraApuDmc
{
    uint8_t enabled, irq_enabled, loop, rate_index, output;
    uint16_t rate, timer;
    uint16_t sample_addr, sample_length, current_addr, bytes_remaining;
    uint8_t buffer, buffer_full, shift, bits_remaining, silence;
} ContraApuDmc;

typedef struct ContraApu
{
    ContraApuPulse pulse[2];
    ContraApuTriangle triangle;
    ContraApuNoise noise;
    ContraApuDmc dmc;
    uint8_t frame_mode;      /* $4017 bit 7: 5-step */
    uint32_t frame_cycle;    /* CPU cycles into the frame sequence */
    uint32_t cycle_parity;   /* pulse/noise/DMC timers tick every other CPU cycle */
    double sample_phase;     /* resampler position */
    double sample_accum;
    uint32_t sample_count_in_accum;
    double hp_prev_in, hp_prev_out; /* DC blocker */
    int16_t samples[CONTRA_AUDIO_MAX_FRAME_SAMPLES];
    size_t sample_count;
} ContraApu;

void contra_apu_reset(ContraApu *apu);
void contra_apu_write(ContraApu *apu, uint16_t addr, uint8_t value);
uint8_t contra_apu_read_status(ContraApu *apu);
/* Clock the APU for `cycles` CPU cycles, appending output samples to
   apu->samples (cleared by the caller between frames). */
void contra_apu_run(ContraApu *apu, uint32_t cycles, ContraApuMemRead read, void *ctx);

#endif
