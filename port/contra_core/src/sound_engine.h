#ifndef CONTRA_SOUND_ENGINE_H
#define CONTRA_SOUND_ENGINE_H

#include <stdint.h>

#include "contra/core.h"

/* Entry points of the bank-1 sound engine (sound_engine.c, generated from
   src/bank1.asm by tools/gen_sound_engine.py). */
typedef enum ContraSoundEntry
{
    CONTRA_SOUND_HANDLE_SOUND_SLOTS,           /* NMI: advance every sound slot */
    CONTRA_SOUND_INIT_SOUND_CODE_VARS,         /* play_sound: start sound code a */
    CONTRA_SOUND_INIT_PULSE_AND_NOISE_CHANNELS /* init_APU_channels */
} ContraSoundEntry;

/* Run one engine entry with the 6502 registers a/x/y (updated on return). */
void contra_sound_exec(ContraCore *core, ContraSoundEntry entry, uint8_t *pa, uint8_t *px, uint8_t *py);

/* Memory as the engine's 6502 sees it (implemented by core.c): RAM, the APU
   registers, PRG ROM with bank 1 mapped at $8000 and bank 7 fixed at $C000. */
uint8_t contra_sound_read(ContraCore *core, uint16_t addr);
void contra_sound_write(ContraCore *core, uint16_t addr, uint8_t value);
/* bank7 play_sound called from inside the engine: a = sound code in, and on
   return a = NMI_CHECK & $7F with n/z from it (x and y are preserved) */
void contra_sound_ext_play_sound(ContraCore *core, uint8_t *a, uint8_t *n, uint8_t *z);

#endif
