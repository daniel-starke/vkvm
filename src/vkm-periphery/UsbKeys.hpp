/**
 * @file UsbKeys.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-10-30
 * @version 2023-10-04
 *
 * @see https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf#page=53
 */
#ifndef __USBKEYS_HPP__
#define __USBKEYS_HPP__


/* USB periphery state */
#define USBSTATE_OFF 0x00
#define USBSTATE_ON 0x01
#define USBSTATE_CONFIGURED 0x02
#define USBSTATE_ON_CONFIGURED 0x03


/* mouse buttons */
#define USBBUTTON_LEFT 0x01
#define USBBUTTON_RIGHT 0x02
#define USBBUTTON_MIDDLE 0x04
#define USBBUTTON_ALL ((USBBUTTON_LEFT) | (USBBUTTON_RIGHT) | (USBBUTTON_MIDDLE))


/* keyboard keys */
#define USBKEY_NO_EVENT 0x00
#define USBKEY_ERROR_ROLL_OVER 0x01
#define USBKEY_POST_FAIL 0x02
#define USBKEY_ERROR_UNDEFINED 0x03
#define USBKEY_A 0x04
#define USBKEY_B 0x05
#define USBKEY_C 0x06
#define USBKEY_D 0x07
#define USBKEY_E 0x08
#define USBKEY_F 0x09
#define USBKEY_G 0x0A
#define USBKEY_H 0x0B
#define USBKEY_I 0x0C
#define USBKEY_J 0x0D
#define USBKEY_K 0x0E
#define USBKEY_L 0x0F
#define USBKEY_M 0x10
#define USBKEY_N 0x11
#define USBKEY_O 0x12
#define USBKEY_P 0x13
#define USBKEY_Q 0x14
#define USBKEY_R 0x15
#define USBKEY_S 0x16
#define USBKEY_T 0x17
#define USBKEY_U 0x18
#define USBKEY_V 0x19
#define USBKEY_W 0x1A
#define USBKEY_X 0x1B
#define USBKEY_Y 0x1C
#define USBKEY_Z 0x1D
#define USBKEY_1 0x1E /* and ! */
#define USBKEY_2 0x1F /* and @ */
#define USBKEY_3 0x20 /* and # */
#define USBKEY_4 0x21 /* and $ */
#define USBKEY_5 0x22 /* and % */
#define USBKEY_6 0x23 /* and ^ */
#define USBKEY_7 0x24 /* and & */
#define USBKEY_8 0x25 /* and * */
#define USBKEY_9 0x26 /* and ( */
#define USBKEY_0 0x27 /* and ) */
#define USBKEY_ENTER 0x28
#define USBKEY_ESCAPE 0x29
#define USBKEY_BACKSPACE 0x2A
#define USBKEY_TAB 0x2B
#define USBKEY_SPACE 0x2C
#define USBKEY_MINUS 0x2D /* and _ */
#define USBKEY_EQUAL 0x2E /* and + */
#define USBKEY_OPEN_BRACKET 0x2F /* [ and { */
#define USBKEY_CLOSE_BRACKET 0x30 /* ] and } */
#define USBKEY_BACKSLASH 0x31 /* and | */
#define USBKEY_NON_US_HASH 0x32 /* and ~ */
#define USBKEY_SEMICOLON 0x33 /* and : */
#define USBKEY_APOSTROPHE 0x34 /* and " */
#define USBKEY_ACCENT 0x35 /* and tilde */
#define USBKEY_COMMA 0x36 /* and < */
#define USBKEY_PERIOD 0x37 /* and > */
#define USBKEY_SLASH 0x38 /* and ? */
#define USBKEY_CAPS_LOCK 0x39
#define USBKEY_F1 0x3A
#define USBKEY_F2 0x3B
#define USBKEY_F3 0x3C
#define USBKEY_F4 0x3D
#define USBKEY_F5 0x3E
#define USBKEY_F6 0x3F
#define USBKEY_F7 0x40
#define USBKEY_F8 0x41
#define USBKEY_F9 0x42
#define USBKEY_F10 0x43
#define USBKEY_F11 0x44
#define USBKEY_F12 0x45
#define USBKEY_PRINT_SCREEN 0x46
#define USBKEY_SCROLL_LOCK 0x47
#define USBKEY_PAUSE 0x48
#define USBKEY_INSERT 0x49
#define USBKEY_HOME 0x4A
#define USBKEY_PAGE_UP 0x4B
#define USBKEY_DELETE 0x4C
#define USBKEY_END 0x4D
#define USBKEY_PAGE_DOWN 0x4E
#define USBKEY_RIGHT_ARROW 0x4F
#define USBKEY_LEFT_ARROW 0x50
#define USBKEY_DOWN_ARROW 0x51
#define USBKEY_UP_ARROW 0x52
#define USBKEY_NUM_LOCK 0x53 /* and clear */
#define USBKEY_KP_DIVIDE 0x54
#define USBKEY_KP_MULTIPLY 0x55
#define USBKEY_KP_SUBTRACT 0x56
#define USBKEY_KP_ADD 0x57
#define USBKEY_KP_ENTER 0x58
#define USBKEY_KP_1 0x59 /* and end */
#define USBKEY_KP_2 0x5A /* and down arrow */
#define USBKEY_KP_3 0x5B /* and page down */
#define USBKEY_KP_4 0x5C /* and left arrow */
#define USBKEY_KP_5 0x5D
#define USBKEY_KP_6 0x5E /* and right arrow */
#define USBKEY_KP_7 0x5F /* and home */
#define USBKEY_KP_8 0x60 /* and up arrow */
#define USBKEY_KP_9 0x61 /* and page up */
#define USBKEY_KP_0 0x62 /* and insert */
#define USBKEY_KP_DECIMAL 0x63 /* and delete */
#define USBKEY_NON_US_BACKSLASH 0x64 /* and | */
#define USBKEY_APPLICATION 0x65
#define USBKEY_POWER 0x66
#define USBKEY_KP_EQUAL 0x67
#define USBKEY_F13 0x68
#define USBKEY_F14 0x69
#define USBKEY_F15 0x6A
#define USBKEY_F16 0x6B
#define USBKEY_F17 0x6C
#define USBKEY_F18 0x6D
#define USBKEY_F19 0x6E
#define USBKEY_F20 0x6F
#define USBKEY_F21 0x70
#define USBKEY_F22 0x71
#define USBKEY_F23 0x72
#define USBKEY_F24 0x73
#define USBKEY_EXECUTE 0x74
#define USBKEY_HELP 0x75
#define USBKEY_MENU 0x76
#define USBKEY_SELECT 0x77
#define USBKEY_STOP 0x78
#define USBKEY_AGAIN 0x79
#define USBKEY_UNDO 0x7A
#define USBKEY_CUT 0x7B
#define USBKEY_COPY 0x7C
#define USBKEY_PASTE 0x7D
#define USBKEY_FIND 0x7E
#define USBKEY_MUTE 0x7F
#define USBKEY_VOLUME_UP 0x80
#define USBKEY_VOLUME_DOWN 0x81
#define USBKEY_LOCKING_CAPS_LOCK 0x82
#define USBKEY_LOCKING_NUM_LOCK 0x83
#define USBKEY_LOCKING_SCROLL_LOCK 0x84
#define USBKEY_KP_COMMA 0x85
#define USBKEY_KP_EQUAL_SIGN 0x86
#define USBKEY_INT_1 0x87 /* romaji */
#define USBKEY_INT_2 0x88 /* kana */
#define USBKEY_INT_3 0x89 /* yen */
#define USBKEY_INT_4 0x8A /* conversion */
#define USBKEY_INT_5 0x8B /* no conversion */
#define USBKEY_INT_6 0x8C /* japanese comma */
#define USBKEY_INT_7 0x8D /* toggle double/single byte mode */
#define USBKEY_INT_8 0x8E /* undefined */
#define USBKEY_INT_9 0x8F /* undefined */
#define USBKEY_IME_KANA 0x88 /* kana */
#define USBKEY_IME_CONVERT 0x8A /* conversion */
#define USBKEY_IME_NONCONVERT 0x8B /* no conversion */
#define USBKEY_LANG_1 0x90 /* hangul */
#define USBKEY_LANG_2 0x91 /* hanja */
#define USBKEY_LANG_3 0x92 /* katakana */
#define USBKEY_LANG_4 0x93 /* hiragana */
#define USBKEY_LANG_5 0x94 /* full/half width toggle */
#define USBKEY_LANG_6 0x95 /* reserved for input method editors */
#define USBKEY_LANG_7 0x96 /* reserved for input method editors */
#define USBKEY_LANG_8 0x97 /* reserved for input method editors */
#define USBKEY_LANG_9 0x98 /* reserved for input method editors */
#define USBKEY_ALT_ERASE 0x99
#define USBKEY_ATTN 0x9A
#define USBKEY_CANCEL 0x9B
#define USBKEY_CLEAR 0x9C
#define USBKEY_PRIOR 0x9D
#define USBKEY_RETURN 0x9E
#define USBKEY_SEPARATOR 0x9F
#define USBKEY_OUT 0xA0
#define USBKEY_OPER 0xA1
#define USBKEY_CLEAR_AGAIN 0xA2
#define USBKEY_CRSEL_PROPS 0xA3
#define USBKEY_EXSEL 0xA4

//#define USBKEY_ 0xA5 /* reserved */
//#define USBKEY_ 0xA6 /* reserved */
//#define USBKEY_ 0xA7 /* reserved */
//#define USBKEY_ 0xA8 /* reserved */
//#define USBKEY_ 0xA9 /* reserved */
//#define USBKEY_ 0xAA /* reserved */
//#define USBKEY_ 0xAB /* reserved */
//#define USBKEY_ 0xAC /* reserved */
//#define USBKEY_ 0xAD /* reserved */
//#define USBKEY_ 0xAE /* reserved */
//#define USBKEY_ 0xAF /* reserved */
#define USBKEY_KP_00 0xB0
#define USBKEY_KP_000 0xB1
#define USBKEY_THOUSENDS_SEP 0xB2
#define USBKEY_DECIMAL_SEP 0xB3
#define USBKEY_CURRENCY_UNIT 0xB4
#define USBKEY_CURRENCY_SUB_UNIT 0xB5
#define USBKEY_KP_OPEN_BRACKET 0xB6 /* ( */
#define USBKEY_KP_CLOSE_BRACKET 0xB7 /* ) */
#define USBKEY_KP_OPEN_CURLY_BRACKET 0xB8
#define USBKEY_KP_CLOSE_CURLY_BRACKET 0xB9
#define USBKEY_KP_TAB 0xBA
#define USBKEY_KP_BACKSPACE 0xBB
#define USBKEY_KP_A 0xBC
#define USBKEY_KP_B 0xBD
#define USBKEY_KP_C 0xBE
#define USBKEY_KP_D 0xBF
#define USBKEY_KP_E 0xC0
#define USBKEY_KP_F 0xC1
#define USBKEY_KP_XOR 0xC2
#define USBKEY_KP_CARET 0xC3
#define USBKEY_KP_PERCENT 0xC4
#define USBKEY_KP_LESS 0xC5
#define USBKEY_KP_GREATER 0xC6
#define USBKEY_KP_AND 0xC7
#define USBKEY_KP_AND2 0xC8
#define USBKEY_KP_OR 0xC9
#define USBKEY_KP_OR2 0xCA
#define USBKEY_KP_COLON 0xCB
#define USBKEY_KP_HASH 0xCC
#define USBKEY_KP_SPACE 0xCD
#define USBKEY_KP_AT 0xCE
#define USBKEY_KP_EXCLAMATION 0xCF
#define USBKEY_KP_MEM_STORE 0xD0
#define USBKEY_KP_MEM_RECALL 0xD1
#define USBKEY_KP_MEM_CLEAR 0xD2
#define USBKEY_KP_MEM_ADD 0xD3
#define USBKEY_KP_MEM_SUB 0xD4
#define USBKEY_KP_MEM_MUL 0xD5
#define USBKEY_KP_MEM_DIV 0xD6
#define USBKEY_KP_PLUS_MINUS 0xD7
#define USBKEY_KP_CLEAR 0xD8
#define USBKEY_KP_CLEAR_ENTRY 0xD9
#define USBKEY_KP_BIN 0xDA
#define USBKEY_KP_OCT 0xDB
#define USBKEY_KP_DEC 0xDC
#define USBKEY_KP_HEX 0xDD
//#define USBKEY_ 0xDE /* reserved */
//#define USBKEY_ 0xDF /* reserved */

#define USBKEY_LEFT_CONTROL 0xE0
#define USBKEY_LEFT_SHIFT 0xE1
#define USBKEY_LEFT_ALT 0xE2
#define USBKEY_LEFT_GUI 0xE3
#define USBKEY_RIGHT_CONTROL 0xE4
#define USBKEY_RIGHT_SHIFT 0xE5
#define USBKEY_RIGHT_ALT 0xE6
#define USBKEY_RIGHT_GUI 0xE7
/* E8..FFFF reserved */


/* keyboard LEDs */
#define USBLED_NUM_LOCK 0x01
#define USBLED_CAPS_LOCK 0x02
#define USBLED_SCROLL_LOCK 0x04
#define USBLED_COMPOSE 0x08
#define USBLED_KANA 0x10
#define USBLED_POWER 0x20
#define USBLED_SHIFT 0x40
#define USBLED_DO_NOT_DISTURB 0x80


/* write modifiers */
#define USBWRITE_NONE 0x00
#define USBWRITE_LEFT_CONTROL 0x01
#define USBWRITE_LEFT_SHIFT 0x02
#define USBWRITE_LEFT_ALT 0x04
#define USBWRITE_RIGHT_CONTROL 0x08
#define USBWRITE_RIGHT_SHIFT 0x10
#define USBWRITE_RIGHT_ALT 0x20
#define USBWRITE_RIGHT_NUM_LOCK 0x40
#define USBWRITE_RIGHT_KANA 0x80


#endif /* __USBKEYS_HPP__ */
