#ifndef __KBD_H__
#define __KBD_H__

#include <tipos.h>

#define KBD_CTRL_PORT 0x64
#define KBD_DATA_PORT 0x60

#define KBD_SC_NONE   0
#define KBD_SC_ESCAPE 0xe0
#define KBD_SC_CTRL   0x1d
#define KBD_SC_ALT    0x38
#define KBD_SC_LSHIFT 0x2a
#define KBD_SC_RSHIFT 0x36

#define KBD_SC_UP     0x48
#define KBD_SC_LEFT   0x4b
#define KBD_SC_RIGHT  0x4d
#define KBD_SC_DOWN   0x50

#define KBD_MD_LCTRL  1
#define KBD_MD_LSHIFT 2
#define KBD_MD_LALT   4
#define KBD_MD_RCTRL  8
#define KBD_MD_RSHIFT 16
#define KBD_MD_RALT   32

#define KBD_SHIFT_ON (kbd_md_state & (KBD_MD_LSHIFT | KBD_MD_RSHIFT))
#define KBD_ALT_ON   (kbd_md_state & (KBD_MD_LALT | KBD_MD_RALT))

#define KBD_LEFT_PRESS  (kbd_sc_buf == KBD_SC_LEFT)
#define KBD_RIGHT_PRESS (kbd_sc_buf == KBD_SC_RIGHT)

// Printable characters map
const char KBD_LOWER_MAP[256];
const char KBD_UPPER_MAP[256];

// Global state variables
uint_8 kbd_sc_buf;     // Raw scancode buffer
uint_8 kbd_md_state;   // Modifier state (CTRL, ALT or SHIFT pressed)
bool   kbd_escaped;    // Escape scancode state
char   kbd_char_buf;   // Buffer for ASCII characters

#endif // __KBD_H__
