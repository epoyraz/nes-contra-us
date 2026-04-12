#ifndef CONTRA_BUTTONS_H
#define CONTRA_BUTTONS_H

/*
 * NES controller bit order in Contra, from MSB to LSB:
 * A, B, Select, Start, Up, Down, Left, Right
 *
 * Source reference:
 * src/bank7.asm, read_controller_state comments around lines 1003-1006.
 */
enum
{
    CONTRA_BUTTON_RIGHT = 0x01u,
    CONTRA_BUTTON_LEFT = 0x02u,
    CONTRA_BUTTON_DOWN = 0x04u,
    CONTRA_BUTTON_UP = 0x08u,
    CONTRA_BUTTON_START = 0x10u,
    CONTRA_BUTTON_SELECT = 0x20u,
    CONTRA_BUTTON_B = 0x40u,
    CONTRA_BUTTON_A = 0x80u
};

#endif
