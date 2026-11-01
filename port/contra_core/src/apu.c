#include "contra/apu.h"

#include <string.h>

#define APU_CPU_HZ 1789773.0

static const uint8_t length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};
static const uint8_t duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 0, 0, 0}, {1, 0, 0, 1, 1, 1, 1, 1}};
static const uint8_t triangle_table[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const uint16_t noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};
static const uint16_t dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54};

void contra_apu_reset(ContraApu *apu)
{
    memset(apu, 0, sizeof(*apu));
    apu->pulse[0].ones_complement = 1u;
    apu->noise.shift = 1u;
    apu->noise.period = noise_period_table[0];
    apu->dmc.rate = dmc_rate_table[0];
    apu->dmc.bits_remaining = 8u;
    apu->dmc.silence = 1u;
}

/* ---- envelope / sweep / length helpers -------------------------------- */

static void clock_envelope(ContraApuEnvelope *env)
{
    if (env->start)
    {
        env->start = 0u;
        env->decay = 15u;
        env->divider = env->period;
        return;
    }
    if (env->divider != 0u)
    {
        --env->divider;
        return;
    }
    env->divider = env->period;
    if (env->decay != 0u)
    {
        --env->decay;
    }
    else if (env->loop)
    {
        env->decay = 15u;
    }
}

static uint8_t envelope_volume(const ContraApuEnvelope *env)
{
    return env->constant ? env->period : env->decay;
}

static uint16_t sweep_target(const ContraApuPulse *p)
{
    const uint16_t change = (uint16_t)(p->period >> p->sweep_shift);

    if (p->sweep_negate)
    {
        return (uint16_t)(p->period - change - (p->ones_complement ? 1u : 0u));
    }
    return (uint16_t)(p->period + change);
}

static int pulse_muted(const ContraApuPulse *p)
{
    return (p->period < 8u) || (!p->sweep_negate && (sweep_target(p) > 0x7FFu));
}

static void clock_sweep(ContraApuPulse *p)
{
    if ((p->sweep_divider == 0u) && p->sweep_enabled && (p->sweep_shift != 0u) && !pulse_muted(p))
    {
        p->period = (uint16_t)(sweep_target(p) & 0x7FFu);
    }
    if ((p->sweep_divider == 0u) || p->sweep_reload)
    {
        p->sweep_divider = p->sweep_period;
        p->sweep_reload = 0u;
    }
    else
    {
        --p->sweep_divider;
    }
}

static void quarter_frame(ContraApu *apu)
{
    ContraApuTriangle *t = &apu->triangle;

    clock_envelope(&apu->pulse[0].env);
    clock_envelope(&apu->pulse[1].env);
    clock_envelope(&apu->noise.env);
    if (t->linear_reload)
    {
        t->linear_counter = t->linear_reload_value;
    }
    else if (t->linear_counter != 0u)
    {
        --t->linear_counter;
    }
    if (!t->control)
    {
        t->linear_reload = 0u;
    }
}

static void half_frame(ContraApu *apu)
{
    int i;

    for (i = 0; i < 2; ++i)
    {
        ContraApuPulse *p = &apu->pulse[i];

        if (!p->env.loop && (p->length != 0u))
        {
            --p->length;
        }
        clock_sweep(p);
    }
    if (!apu->triangle.control && (apu->triangle.length != 0u))
    {
        --apu->triangle.length;
    }
    if (!apu->noise.env.loop && (apu->noise.length != 0u))
    {
        --apu->noise.length;
    }
}

/* ---- registers ------------------------------------------------------------ */

static void dmc_restart(ContraApuDmc *d)
{
    d->current_addr = d->sample_addr;
    d->bytes_remaining = d->sample_length;
}

void contra_apu_write(ContraApu *apu, uint16_t addr, uint8_t value)
{
    if ((addr >= 0x4000u) && (addr <= 0x4007u))
    {
        ContraApuPulse *p = &apu->pulse[(addr - 0x4000u) >> 2];

        switch (addr & 3u)
        {
            case 0u:
                p->duty = (uint8_t)(value >> 6);
                p->env.loop = (uint8_t)((value >> 5) & 1u);
                p->env.constant = (uint8_t)((value >> 4) & 1u);
                p->env.period = (uint8_t)(value & 15u);
                break;
            case 1u:
                p->sweep_enabled = (uint8_t)(value >> 7);
                p->sweep_period = (uint8_t)((value >> 4) & 7u);
                p->sweep_negate = (uint8_t)((value >> 3) & 1u);
                p->sweep_shift = (uint8_t)(value & 7u);
                p->sweep_reload = 1u;
                break;
            case 2u:
                p->period = (uint16_t)((p->period & 0x700u) | value);
                break;
            default:
                p->period = (uint16_t)((p->period & 0xFFu) | ((value & 7u) << 8));
                if (p->enabled)
                {
                    p->length = length_table[value >> 3];
                }
                p->step = 0u;
                p->env.start = 1u;
                break;
        }
        return;
    }
    switch (addr)
    {
        case 0x4008u:
            apu->triangle.control = (uint8_t)(value >> 7);
            apu->triangle.linear_reload_value = (uint8_t)(value & 0x7Fu);
            break;
        case 0x400Au:
            apu->triangle.period = (uint16_t)((apu->triangle.period & 0x700u) | value);
            break;
        case 0x400Bu:
            apu->triangle.period = (uint16_t)((apu->triangle.period & 0xFFu) | ((value & 7u) << 8));
            if (apu->triangle.enabled)
            {
                apu->triangle.length = length_table[value >> 3];
            }
            apu->triangle.linear_reload = 1u;
            break;
        case 0x400Cu:
            apu->noise.env.loop = (uint8_t)((value >> 5) & 1u);
            apu->noise.env.constant = (uint8_t)((value >> 4) & 1u);
            apu->noise.env.period = (uint8_t)(value & 15u);
            break;
        case 0x400Eu:
            apu->noise.mode = (uint8_t)(value >> 7);
            apu->noise.period = noise_period_table[value & 15u];
            break;
        case 0x400Fu:
            if (apu->noise.enabled)
            {
                apu->noise.length = length_table[value >> 3];
            }
            apu->noise.env.start = 1u;
            break;
        case 0x4010u:
            apu->dmc.irq_enabled = (uint8_t)(value >> 7);
            apu->dmc.loop = (uint8_t)((value >> 6) & 1u);
            apu->dmc.rate_index = (uint8_t)(value & 15u);
            apu->dmc.rate = dmc_rate_table[value & 15u];
            break;
        case 0x4011u:
            apu->dmc.output = (uint8_t)(value & 0x7Fu);
            break;
        case 0x4012u:
            apu->dmc.sample_addr = (uint16_t)(0xC000u + (uint16_t)value * 64u);
            break;
        case 0x4013u:
            apu->dmc.sample_length = (uint16_t)((uint16_t)value * 16u + 1u);
            break;
        case 0x4015u:
            apu->pulse[0].enabled = (uint8_t)(value & 1u);
            apu->pulse[1].enabled = (uint8_t)((value >> 1) & 1u);
            apu->triangle.enabled = (uint8_t)((value >> 2) & 1u);
            apu->noise.enabled = (uint8_t)((value >> 3) & 1u);
            if (!apu->pulse[0].enabled) apu->pulse[0].length = 0u;
            if (!apu->pulse[1].enabled) apu->pulse[1].length = 0u;
            if (!apu->triangle.enabled) apu->triangle.length = 0u;
            if (!apu->noise.enabled) apu->noise.length = 0u;
            if ((value & 0x10u) == 0u)
            {
                apu->dmc.bytes_remaining = 0u;
            }
            else if (apu->dmc.bytes_remaining == 0u)
            {
                dmc_restart(&apu->dmc);
            }
            break;
        case 0x4017u:
            apu->frame_mode = (uint8_t)(value >> 7);
            apu->frame_cycle = 0u;
            if (apu->frame_mode)
            {
                quarter_frame(apu);
                half_frame(apu);
            }
            break;
        default:
            break;
    }
}

uint8_t contra_apu_read_status(ContraApu *apu)
{
    return (uint8_t)((apu->pulse[0].length ? 0x01u : 0u) | (apu->pulse[1].length ? 0x02u : 0u) |
                     (apu->triangle.length ? 0x04u : 0u) | (apu->noise.length ? 0x08u : 0u) |
                     (apu->dmc.bytes_remaining ? 0x10u : 0u));
}

/* ---- clocking ---------------------------------------------------------- */

static void clock_frame_sequencer(ContraApu *apu)
{
    ++apu->frame_cycle;
    switch (apu->frame_cycle)
    {
        case 7457u: quarter_frame(apu); break;
        case 14913u: quarter_frame(apu); half_frame(apu); break;
        case 22371u: quarter_frame(apu); break;
        case 29829u:
            if (!apu->frame_mode)
            {
                quarter_frame(apu);
                half_frame(apu);
            }
            break;
        case 29830u:
            if (!apu->frame_mode)
            {
                apu->frame_cycle = 0u;
            }
            break;
        case 37281u: quarter_frame(apu); half_frame(apu); break;
        case 37282u: apu->frame_cycle = 0u; break;
        default: break;
    }
}

static void clock_dmc(ContraApu *apu, ContraApuMemRead read, void *ctx)
{
    ContraApuDmc *d = &apu->dmc;

    if (!d->buffer_full && (d->bytes_remaining != 0u) && (read != NULL))
    {
        d->buffer = read(ctx, d->current_addr);
        d->buffer_full = 1u;
        d->current_addr = (uint16_t)((d->current_addr == 0xFFFFu) ? 0x8000u : (d->current_addr + 1u));
        if (--d->bytes_remaining == 0u && d->loop)
        {
            dmc_restart(d);
        }
    }
    if (d->timer != 0u)
    {
        --d->timer;
        return;
    }
    d->timer = (uint16_t)(d->rate - 1u);
    if (!d->silence)
    {
        if (d->shift & 1u)
        {
            if (d->output <= 125u) d->output = (uint8_t)(d->output + 2u);
        }
        else if (d->output >= 2u)
        {
            d->output = (uint8_t)(d->output - 2u);
        }
    }
    d->shift >>= 1;
    if (--d->bits_remaining == 0u)
    {
        d->bits_remaining = 8u;
        if (d->buffer_full)
        {
            d->shift = d->buffer;
            d->buffer_full = 0u;
            d->silence = 0u;
        }
        else
        {
            d->silence = 1u;
        }
    }
}

static double mix(const ContraApu *apu)
{
    double p = 0.0;
    double tnd;
    int i;
    unsigned t;
    unsigned n;
    unsigned d;

    for (i = 0; i < 2; ++i)
    {
        const ContraApuPulse *pu = &apu->pulse[i];

        if ((pu->length != 0u) && !pulse_muted(pu) && duty_table[pu->duty][pu->step])
        {
            p += envelope_volume(&pu->env);
        }
    }
    t = triangle_table[apu->triangle.step];
    n = ((apu->noise.length != 0u) && !(apu->noise.shift & 1u)) ? envelope_volume(&apu->noise.env) : 0u;
    d = apu->dmc.output;
    p = (p > 0.0) ? 95.88 / (8128.0 / p + 100.0) : 0.0;
    tnd = (double)t / 8227.0 + (double)n / 12241.0 + (double)d / 22638.0;
    tnd = (tnd > 0.0) ? 159.79 / (1.0 / tnd + 100.0) : 0.0;
    return p + tnd;
}

void contra_apu_run(ContraApu *apu, uint32_t cycles, ContraApuMemRead read, void *ctx)
{
    const double cycles_per_sample = APU_CPU_HZ / (double)CONTRA_AUDIO_SAMPLE_RATE;
    uint32_t i;

    for (i = 0u; i < cycles; ++i)
    {
        ContraApuTriangle *tr = &apu->triangle;

        clock_frame_sequencer(apu);
        /* triangle and DMC timers run at the CPU rate */
        if (tr->timer == 0u)
        {
            tr->timer = tr->period;
            if ((tr->linear_counter != 0u) && (tr->length != 0u) && (tr->period >= 2u))
            {
                tr->step = (uint8_t)((tr->step + 1u) & 31u);
            }
        }
        else
        {
            --tr->timer;
        }
        clock_dmc(apu, read, ctx);
        /* pulse and noise timers run at the APU rate (every other cycle) */
        apu->cycle_parity ^= 1u;
        if (apu->cycle_parity)
        {
            int k;

            for (k = 0; k < 2; ++k)
            {
                ContraApuPulse *p = &apu->pulse[k];

                if (p->timer == 0u)
                {
                    p->timer = p->period;
                    p->step = (uint8_t)((p->step + 1u) & 7u);
                }
                else
                {
                    --p->timer;
                }
            }
            if (apu->noise.timer == 0u)
            {
                const uint16_t s = apu->noise.shift;
                const uint16_t feedback = (uint16_t)((s ^ (s >> (apu->noise.mode ? 6u : 1u))) & 1u);

                apu->noise.timer = (uint16_t)(apu->noise.period / 2u - 1u);
                apu->noise.shift = (uint16_t)((s >> 1) | (feedback << 14));
            }
            else
            {
                --apu->noise.timer;
            }
        }

        apu->sample_accum += mix(apu);
        ++apu->sample_count_in_accum;
        apu->sample_phase += 1.0;
        if (apu->sample_phase >= cycles_per_sample)
        {
            const double in = apu->sample_accum / (double)apu->sample_count_in_accum;
            /* DC blocker (the NES output stage is AC coupled) */
            const double out = in - apu->hp_prev_in + 0.996 * apu->hp_prev_out;
            double s = out * 1.6 * 32767.0;

            apu->hp_prev_in = in;
            apu->hp_prev_out = out;
            apu->sample_phase -= cycles_per_sample;
            apu->sample_accum = 0.0;
            apu->sample_count_in_accum = 0u;
            if (s > 32767.0) s = 32767.0;
            if (s < -32768.0) s = -32768.0;
            if (apu->sample_count < CONTRA_AUDIO_MAX_FRAME_SAMPLES)
            {
                apu->samples[apu->sample_count++] = (int16_t)s;
            }
        }
    }
}
