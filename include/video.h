/*
* video.h ~ Video driver (header)
*
* Copyright 2010 Damián Emiliano Silvani <dsilvani@gmail.com>,
*                Patricio Reboratti <darthpolly@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef __VIDEO_H__
#define __VIDEO_H__


#include "stdarg.h"
#include "klib.h"


#define VGA_TEXT_BUFFER 0xb8000
#define MAX_COLS 80
#define MAX_ROWS 25

// Colors
#define C_BLACK   0x00
#define C_RED     0x01
#define C_GREEN   0x02
#define C_YELLOW  0x03
#define C_BLUE    0x04
#define C_MAGENTA 0x05
#define C_CYAN    0x06
#define C_WHITE   0x07

#define C_BRIGHT_BLACK   0x08
#define C_BRIGHT_RED     0x09
#define C_BRIGHT_GREEN   0x0A
#define C_BRIGHT_YELLOW  0x0B
#define C_BRIGHT_BLUE    0x0C
#define C_BRIGHT_MAGENTA 0x0D
#define C_BRIGHT_CYAN    0x0E
#define C_BRIGHT_WHITE   0x0F

#define ASCII_0 0x30
#define ASCII_a 0x61

#define TAB_WIDTH 4


void clear(void);

int putchar(const char c);
int printf(const char* format, ...);

void set_forecolor(const char color);
void set_backcolor(const char color);
void clear_colors(void);

// TODO: Encontrar un mejor nombre para esta función
// lo que hace: recibe una representación en char de un número hexadecimal
// y devuelve el valor de ese hexadecimal casteado a char.
const char char_to_hex(const char val);

#endif /* __VIDEO_H__ */
