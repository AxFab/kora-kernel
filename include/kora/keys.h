/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#ifndef _KORA_KEYS_H
#define _KORA_KEYS_H  1

/* Control characters */
#define KEY_NUL  0
#define KEY_SOH  1
#define KEY_STX  2
#define KEY_ETX  3
#define KEY_EOT  4
#define KEY_ENQ  5
#define KEY_ACK  6
#define KEY_BEL  7
#define KEY_BS  8
#define KEY_TAB  9
#define KEY_LF  10
#define KEY_VT  11
#define KEY_FF  12
#define KEY_CR  13
#define KEY_SO  14
#define KEY_SI  15

#define KEY_DLE  16
#define KEY_DC1  17
#define KEY_DC2  18
#define KEY_DC3  19
#define KEY_DC4  20
#define KEY_NAK  21
#define KEY_SYN  22
#define KEY_ETB  23
#define KEY_CAN  24
#define KEY_EM  25
#define KEY_SUB  26
#define KEY_ESC  27
#define KEY_FS  28
#define KEY_GS  29
#define KEY_RS  30
#define KEY_US  31


/*  */
#define _KeyCode(n)  (n|0x100000)

#define KEY_CODE_RETURN  KEY_LF
#define KEY_CODE_ESCAPE  KEY_ESC
#define KEY_CODE_BACKSPACE  KEY_BS
#define KEY_CODE_TAB  KEY_TAB
#define KEY_CODE_SPACE  32

#define KEY_CODE_CAPSLOCK  _KeyCode(57)

#define KEY_CODE_F1  _KeyCode(58)
#define KEY_CODE_F2  _KeyCode(59)
#define KEY_CODE_F3  _KeyCode(60)
#define KEY_CODE_F4  _KeyCode(61)
#define KEY_CODE_F5  _KeyCode(62)
#define KEY_CODE_F6  _KeyCode(63)
#define KEY_CODE_F7  _KeyCode(64)
#define KEY_CODE_F8  _KeyCode(65)
#define KEY_CODE_F9  _KeyCode(66)
#define KEY_CODE_F10  _KeyCode(67)
#define KEY_CODE_F11  _KeyCode(68)
#define KEY_CODE_F12  _KeyCode(69)

#define KEY_CODE_PRINTSCREEN  _KeyCode(70)
#define KEY_CODE_SCROLLLOCK  _KeyCode(71)
#define KEY_CODE_PAUSE  _KeyCode(72)
#define KEY_CODE_INSERT  _KeyCode(73)

#define KEY_CODE_HOME  _KeyCode(74)
#define KEY_CODE_PAGEUP  _KeyCode(75)
#define KEY_CODE_DELETE  _KeyCode(76)
#define KEY_CODE_END  _KeyCode(77)
#define KEY_CODE_PAGEDOWN  _KeyCode(78)
#define KEY_CODE_RIGHT  _KeyCode(79)
#define KEY_CODE_LEFT  _KeyCode(80)
#define KEY_CODE_DOWN  _KeyCode(81)
#define KEY_CODE_UP  _KeyCode(82)

#define KEY_CODE_NUMLOCK  _KeyCode(83)

#define KEY_CODE_KP_DIVIDE  _KeyCode(84)
#define KEY_CODE_KP_MULTIPLY  _KeyCode(85)
#define KEY_CODE_KP_MINUS  _KeyCode(86)
#define KEY_CODE_KP_PLUS  _KeyCode(87)
#define KEY_CODE_KP_ENTER  _KeyCode(88)
#define KEY_CODE_KP_1  _KeyCode(89)
#define KEY_CODE_KP_2  _KeyCode(90)
#define KEY_CODE_KP_3  _KeyCode(91)
#define KEY_CODE_KP_4  _KeyCode(92)
#define KEY_CODE_KP_5  _KeyCode(93)
#define KEY_CODE_KP_6  _KeyCode(94)
#define KEY_CODE_KP_7  _KeyCode(95)
#define KEY_CODE_KP_8  _KeyCode(96)
#define KEY_CODE_KP_9  _KeyCode(97)
#define KEY_CODE_KP_0  _KeyCode(98)
#define KEY_CODE_KP_PERIOD  _KeyCode(99)


#define KEY_CODE_LCTRL  _KeyCode(224)
#define KEY_CODE_LSHIFT  _KeyCode(225)
#define KEY_CODE_LALT  _KeyCode(226)
#define KEY_CODE_LGUI  _KeyCode(227)
#define KEY_CODE_RCTRL  _KeyCode(228)
#define KEY_CODE_RSHIFT  _KeyCode(229)
#define KEY_CODE_RALT  _KeyCode(230)
#define KEY_CODE_RGUI  _KeyCode(231)

#define KEY_CODE_BREAK  _KeyCode(232)



/* - */
#define KEY_STATUS_LSHIFT  1
#define KEY_STATUS_RSHIFT  2
#define KEY_STATUS_LCTRL  4
#define KEY_STATUS_RCTRL  8
#define KEY_STATUS_LALT  16
#define KEY_STATUS_RALT  32
#define KEY_STATUS_LGUI  64
#define KEY_STATUS_RGUI  128
#define KEY_STATUS_CAPSLOCK  256

#define KEY_STATUS_SHIFT  (KEY_STATUS_LSHIFT | KEY_STATUS_RSHIFT)
#define KEY_STATUS_CTRL  (KEY_STATUS_LCTRL | KEY_STATUS_RCTRL)
#define KEY_STATUS_CALT  (KEY_STATUS_LALT | KEY_STATUS_RALT)
#define KEY_STATUS_GUI  (KEY_STATUS_LGUI | KEY_STATUS_RGUI)


extern int keyboard_layout_US[0x90][4];

int keyboard_down(int key, int *status, int *key2);
int keyboard_up(int key, int *status);

#endif  /* _KORA_KEYS_H */

