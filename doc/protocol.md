Serial Protocol
===============

The VKVM control and periphery communicate via USB CDC like a serial device.
This makes it necessary to use a proper framing protocol.
All related function to this are included in [Framing.hpp](../src/vkm-periphery/Framing.hpp).
The framing is done similar to [RFC 1662][RFC1662] with `0x7E` as start/stop byte,
`0x7D` as escape byte, which indicates that the next byte is XOR `0x20` to quote bytes
used for framing. The actual frame has the following structure with all bytes except
STX/ETX being properly quoted.

|Byte |Size |Field|Description                     |
|----:|----:|-----|--------------------------------|
|    0|    1|STX  |Start of frame (`0x7E`).        |
|    1|    1|SEQ  |Frame sequence number.          |
|    2|  N-5|DATA |User payload.                   |
|  N-3|    2|CRC16|CRC16 over the unquoted payload.|
|  N-1|    1|ETX  |End of frame (`0x7E`).          |

CRC16-CCITT like in HDLC (see [RFC 1662][RFC1662]) is being used here.

[RFC1662]: https://www.rfc-editor.org/rfc/rfc1662

### Constants

|Field        |Name                          |Value |Description                              |
|-------------|------------------------------|-----:|-----------------------------------------|
|RES          |S_OK                          |  0x00|The request has been processed.          |
|RES          |I_USB_STATE_UPDATE            |  0x40|Updated event of the USB periphery state.|
|RES          |I_LED_UPDATE                  |  0x41|Updated event of the LED states.         |
|RES          |D_MESSAGE                     |  0x60|Debug message.                           |
|RES          |E_BROKEN_FRAME                |  0x80|Received broken frame.                   |
|RES          |E_UNSUPPORTED_REQ_TYPE        |  0x81|Received unsupported request type.       |
|RES          |E_INVALID_REQ_TYPE            |  0x82|Received invalid request type.           |
|RES          |E_INVALID_FIELD_VALUE         |  0x83|Received invalid field value.            |
|RES          |E_HOST_WRITE_ERROR            |  0x85|Failed write to remote host.             |
|REQ          |GET_PROTOCOL_VERSION          |  0x00|Request protocol version.                |
|REQ          |GET_ALIVE                     |  0x01|Request keep alive.                      |
|REQ          |GET_USB_STATE                 |  0x02|Request USB periphery state.             |
|REQ          |GET_KEYBOARD_LEDS             |  0x03|Request keyboard LED states.             |
|REQ          |SET_KEYBOARD_DOWN             |  0x04|Push keyboard key down.                  |
|REQ          |SET_KEYBOARD_UP               |  0x05|Release keyboard key.                    |
|REQ          |SET_KEYBOARD_ALL_UP           |  0x06|Release all keyboard keys.               |
|REQ          |SET_KEYBOARD_PUSH             |  0x07|Single push keyboard key.                |
|REQ          |SET_KEYBOARD_WRITE            |  0x08|Write keyboard keys.                     |
|REQ          |SET_MOUSE_BUTTON_DOWN         |  0x09|Push mouse button down.                  |
|REQ          |SET_MOUSE_BUTTON_UP           |  0x0A|Release mouse button.                    |
|REQ          |SET_MOUSE_BUTTON_ALL_UP       |  0x0B|Release all mouse buttons.               |
|REQ          |SET_MOUSE_BUTTON_PUSH         |  0x0C|Single push mouse button.                |
|REQ          |SET_MOUSE_MOVE_ABS            |  0x0D|Absolute mouse movement.                 |
|REQ          |SET_MOUSE_MOVE_REL            |  0x0E|Relative mouse movement.                 |
|REQ          |SET_MOUSE_SCROLL              |  0x0F|Mouse wheel change.                      |
|USB          |USBSTATE_OFF                  |  0x00|USB periphery is physically disconnected.|
|USB          |USBSTATE_ON                   |  0x01|USB periphery is physically connected.   |
|USB          |USBSTATE_CONFIGURED           |  0x02|USB periphery is configured by host.     |
|USB          |USBSTATE_ON_CONFIGURED        |  0x03|USB periphery is fully functional.       |
|LED&#185;    |USBLED_NUM_LOCK               |  0x01|Num lock key status.                     |
|LED&#185;    |USBLED_CAPS_LOCK              |  0x02|Caps lock key status.                    |
|LED&#185;    |USBLED_SCROLL_LOCK            |  0x04|Scroll lock key status.                  |
|LED&#185;    |USBLED_COMPOSE                |  0x08|Composition mode enabled status.         |
|LED&#185;    |USBLED_KANA                   |  0x10|Kana key status.                         |
|LED&#185;    |USBLED_POWER                  |  0x20|Power status.                            |
|LED&#185;    |USBLED_SHIFT                  |  0x40|Shift function status.                   |
|LED&#185;    |USBLED_DO_NOT_DISTURB         |  0x80|Shift function status.                   |
|KEY&#178;    |USBKEY_NO_EVENT               |  0x00|No keyboard event indicator.             |
|KEY&#178;    |USBKEY_ERROR_ROLL_OVER        |  0x01|Keyboard ErrorRollOver                   |
|KEY&#178;    |USBKEY_POST_FAIL              |  0x02|Keyboard POSTFail                        |
|KEY&#178;    |USBKEY_ERROR_UNDEFINED        |  0x03|Keyboard ErrorUndeﬁned                   |
|KEY&#178;    |USBKEY_A                      |  0x04|Keyboard a and A                         |
|KEY&#178;    |USBKEY_B                      |  0x05|Keyboard b and B                         |
|KEY&#178;    |USBKEY_C                      |  0x06|Keyboard c and C                         |
|KEY&#178;    |USBKEY_D                      |  0x07|Keyboard d and D                         |
|KEY&#178;    |USBKEY_E                      |  0x08|Keyboard e and E                         |
|KEY&#178;    |USBKEY_F                      |  0x09|Keyboard f and F                         |
|KEY&#178;    |USBKEY_G                      |  0x0A|Keyboard g and G                         |
|KEY&#178;    |USBKEY_H                      |  0x0B|Keyboard h and H                         |
|KEY&#178;    |USBKEY_I                      |  0x0C|Keyboard i and I                         |
|KEY&#178;    |USBKEY_J                      |  0x0D|Keyboard j and J                         |
|KEY&#178;    |USBKEY_K                      |  0x0E|Keyboard k and K                         |
|KEY&#178;    |USBKEY_L                      |  0x0F|Keyboard l and L                         |
|KEY&#178;    |USBKEY_M                      |  0x10|Keyboard m and M                         |
|KEY&#178;    |USBKEY_N                      |  0x11|Keyboard n and N                         |
|KEY&#178;    |USBKEY_O                      |  0x12|Keyboard o and O                         |
|KEY&#178;    |USBKEY_P                      |  0x13|Keyboard p and P                         |
|KEY&#178;    |USBKEY_Q                      |  0x14|Keyboard q and Q                         |
|KEY&#178;    |USBKEY_R                      |  0x15|Keyboard r and R                         |
|KEY&#178;    |USBKEY_S                      |  0x16|Keyboard s and S                         |
|KEY&#178;    |USBKEY_T                      |  0x17|Keyboard t and T                         |
|KEY&#178;    |USBKEY_U                      |  0x18|Keyboard u and U                         |
|KEY&#178;    |USBKEY_V                      |  0x19|Keyboard v and V                         |
|KEY&#178;    |USBKEY_W                      |  0x1A|Keyboard w and W                         |
|KEY&#178;    |USBKEY_X                      |  0x1B|Keyboard x and X                         |
|KEY&#178;    |USBKEY_Y                      |  0x1C|Keyboard y and Y                         |
|KEY&#178;    |USBKEY_Z                      |  0x1D|Keyboard z and Z                         |
|KEY&#178;    |USBKEY_1                      |  0x1E|Keyboard 1 and !                         |
|KEY&#178;    |USBKEY_2                      |  0x1F|Keyboard 2 and @                         |
|KEY&#178;    |USBKEY_3                      |  0x20|Keyboard 3 and #                         |
|KEY&#178;    |USBKEY_4                      |  0x21|Keyboard 4 and $                         |
|KEY&#178;    |USBKEY_5                      |  0x22|Keyboard 5 and %                         |
|KEY&#178;    |USBKEY_6                      |  0x23|Keyboard 6 and ^                         |
|KEY&#178;    |USBKEY_7                      |  0x24|Keyboard 7 and &                         |
|KEY&#178;    |USBKEY_8                      |  0x25|Keyboard 8 and *                         |
|KEY&#178;    |USBKEY_9                      |  0x26|Keyboard 9 and (                         |
|KEY&#178;    |USBKEY_0                      |  0x27|Keyboard 0 and )                         |
|KEY&#178;    |USBKEY_ENTER                  |  0x28|Keyboard Return (ENTER)                  |
|KEY&#178;    |USBKEY_ESCAPE                 |  0x29|Keyboard ESCAPE                          |
|KEY&#178;    |USBKEY_BACKSPACE              |  0x2A|Keyboard DELETE (Backspace)              |
|KEY&#178;    |USBKEY_TAB                    |  0x2B|Keyboard Tab                             |
|KEY&#178;    |USBKEY_SPACE                  |  0x2C|Keyboard Spacebar                        |
|KEY&#178;    |USBKEY_MINUS                  |  0x2D|Keyboard - and (underscore)              |
|KEY&#178;    |USBKEY_EQUAL                  |  0x2E|Keyboard = and +                         |
|KEY&#178;    |USBKEY_OPEN_BRACKET           |  0x2F|Keyboard \[ and {                        |
|KEY&#178;    |USBKEY_CLOSE_BRACKET          |  0x30|Keyboard ] and }                         |
|KEY&#178;    |USBKEY_BACKSLASH              |  0x31|Keyboard \\ and &#124;                   |
|KEY&#178;    |USBKEY_NON_US_HASH            |  0x32|Keyboard Non-US # and ˜                   |
|KEY&#178;    |USBKEY_SEMICOLON              |  0x33|Keyboard ; and :                         |
|KEY&#178;    |USBKEY_APOSTROPHE             |  0x34|Keyboard ‘ and “                         |
|KEY&#178;    |USBKEY_ACCENT                 |  0x35|Keyboard Grave Accent and Ti             |
|KEY&#178;    |USBKEY_COMMA                  |  0x36|Keyboard , and <                         |
|KEY&#178;    |USBKEY_PERIOD                 |  0x37|Keyboard . and >                         |
|KEY&#178;    |USBKEY_SLASH                  |  0x38|Keyboard / and ?                         |
|KEY&#178;    |USBKEY_CAPS_LOCK              |  0x39|Keyboard Caps Lock                       |
|KEY&#178;    |USBKEY_F1                     |  0x3A|Keyboard F1                              |
|KEY&#178;    |USBKEY_F2                     |  0x3B|Keyboard F2                              |
|KEY&#178;    |USBKEY_F3                     |  0x3C|Keyboard F3                              |
|KEY&#178;    |USBKEY_F4                     |  0x3D|Keyboard F4                              |
|KEY&#178;    |USBKEY_F5                     |  0x3E|Keyboard F5                              |
|KEY&#178;    |USBKEY_F6                     |  0x3F|Keyboard F6                              |
|KEY&#178;    |USBKEY_F7                     |  0x40|Keyboard F7                              |
|KEY&#178;    |USBKEY_F8                     |  0x41|Keyboard F8                              |
|KEY&#178;    |USBKEY_F9                     |  0x42|Keyboard F9                              |
|KEY&#178;    |USBKEY_F10                    |  0x43|Keyboard F10                             |
|KEY&#178;    |USBKEY_F11                    |  0x44|Keyboard F11                             |
|KEY&#178;    |USBKEY_F12                    |  0x45|Keyboard F12                             |
|KEY&#178;    |USBKEY_PRINT_SCREEN           |  0x46|Keyboard PrintScreen                     |
|KEY&#178;    |USBKEY_SCROLL_LOCK            |  0x47|Keyboard Scroll Lock                     |
|KEY&#178;    |USBKEY_PAUSE                  |  0x48|Keyboard Pause                           |
|KEY&#178;    |USBKEY_INSERT                 |  0x49|Keyboard Insert                          |
|KEY&#178;    |USBKEY_HOME                   |  0x4A|Keyboard Home                            |
|KEY&#178;    |USBKEY_PAGE_UP                |  0x4B|Keyboard PageUp                          |
|KEY&#178;    |USBKEY_DELETE                 |  0x4C|Keyboard Delete Forward                  |
|KEY&#178;    |USBKEY_END                    |  0x4D|Keyboard End                             |
|KEY&#178;    |USBKEY_PAGE_DOWN              |  0x4E|Keyboard PageDown                        |
|KEY&#178;    |USBKEY_RIGHT_ARROW            |  0x4F|Keyboard RightArrow                      |
|KEY&#178;    |USBKEY_LEFT_ARROW             |  0x50|Keyboard LeftArrow                       |
|KEY&#178;    |USBKEY_DOWN_ARROW             |  0x51|Keyboard DownArrow                       |
|KEY&#178;    |USBKEY_UP_ARROW               |  0x52|Keyboard UpArrow                         |
|KEY&#178;    |USBKEY_NUM_LOCK               |  0x53|Keypad Num Lock and Clear                |
|KEY&#178;    |USBKEY_KP_DIVIDE              |  0x54|Keypad /                                 |
|KEY&#178;    |USBKEY_KP_MULTIPLY            |  0x55|Keypad *                                 |
|KEY&#178;    |USBKEY_KP_SUBTRACT            |  0x56|Keypad -                                 |
|KEY&#178;    |USBKEY_KP_ADD                 |  0x57|Keypad +                                 |
|KEY&#178;    |USBKEY_KP_ENTER               |  0x58|Keypad ENTER                             |
|KEY&#178;    |USBKEY_KP_1                   |  0x59|Keypad 1 and End                         |
|KEY&#178;    |USBKEY_KP_2                   |  0x5A|Keypad 2 and Down Arrow                  |
|KEY&#178;    |USBKEY_KP_3                   |  0x5B|Keypad 3 and PageDn                      |
|KEY&#178;    |USBKEY_KP_4                   |  0x5C|Keypad 4 and Left Arrow                  |
|KEY&#178;    |USBKEY_KP_5                   |  0x5D|Keypad 5                                 |
|KEY&#178;    |USBKEY_KP_6                   |  0x5E|Keypad 6 and Right Arrow                 |
|KEY&#178;    |USBKEY_KP_7                   |  0x5F|Keypad 7 and Home                        |
|KEY&#178;    |USBKEY_KP_8                   |  0x60|Keypad 8 and Up Arrow                    |
|KEY&#178;    |USBKEY_KP_9                   |  0x61|Keypad 9 and PageUp                      |
|KEY&#178;    |USBKEY_KP_0                   |  0x62|Keypad 0 and Insert                      |
|KEY&#178;    |USBKEY_KP_DECIMAL             |  0x63|Keypad . and Delete                      |
|KEY&#178;    |USBKEY_NON_US_BACKSLASH       |  0x64|Keyboard Non-US \\ and &#124;            |
|KEY&#178;    |USBKEY_APPLICATION            |  0x65|Keyboard Application                     |
|KEY&#178;    |USBKEY_POWER                  |  0x66|Keyboard Power                           |
|KEY&#178;    |USBKEY_KP_EQUAL               |  0x67|Keypad =                                 |
|KEY&#178;    |USBKEY_F13                    |  0x68|Keyboard F13                             |
|KEY&#178;    |USBKEY_F14                    |  0x69|Keyboard F14                             |
|KEY&#178;    |USBKEY_F15                    |  0x6A|Keyboard F15                             |
|KEY&#178;    |USBKEY_F16                    |  0x6B|Keyboard F16                             |
|KEY&#178;    |USBKEY_F17                    |  0x6C|Keyboard F17                             |
|KEY&#178;    |USBKEY_F18                    |  0x6D|Keyboard F18                             |
|KEY&#178;    |USBKEY_F19                    |  0x6E|Keyboard F19                             |
|KEY&#178;    |USBKEY_F20                    |  0x6F|Keyboard F20                             |
|KEY&#178;    |USBKEY_F21                    |  0x70|Keyboard F21                             |
|KEY&#178;    |USBKEY_F22                    |  0x71|Keyboard F22                             |
|KEY&#178;    |USBKEY_F23                    |  0x72|Keyboard F23                             |
|KEY&#178;    |USBKEY_F24                    |  0x73|Keyboard F24                             |
|KEY&#178;    |USBKEY_EXECUTE                |  0x74|Keyboard Execute                         |
|KEY&#178;    |USBKEY_HELP                   |  0x75|Keyboard Help                            |
|KEY&#178;    |USBKEY_MENU                   |  0x76|Keyboard Menu                            |
|KEY&#178;    |USBKEY_SELECT                 |  0x77|Keyboard Select                          |
|KEY&#178;    |USBKEY_STOP                   |  0x78|Keyboard Stop                            |
|KEY&#178;    |USBKEY_AGAIN                  |  0x79|Keyboard Again                           |
|KEY&#178;    |USBKEY_UNDO                   |  0x7A|Keyboard Undo                            |
|KEY&#178;    |USBKEY_CUT                    |  0x7B|Keyboard Cut                             |
|KEY&#178;    |USBKEY_COPY                   |  0x7C|Keyboard Copy                            |
|KEY&#178;    |USBKEY_PASTE                  |  0x7D|Keyboard Paste                           |
|KEY&#178;    |USBKEY_FIND                   |  0x7E|Keyboard Find                            |
|KEY&#178;    |USBKEY_MUTE                   |  0x7F|Keyboard Mute                            |
|KEY&#178;    |USBKEY_VOLUME_UP              |  0x80|Keyboard Volume Up                       |
|KEY&#178;    |USBKEY_VOLUME_DOWN            |  0x81|Keyboard Volume Down                     |
|KEY&#178;    |USBKEY_LOCKING_CAPS_LOCK      |  0x82|Keyboard Locking Caps Lock               |
|KEY&#178;    |USBKEY_LOCKING_NUM_LOCK       |  0x83|Keyboard Locking Num Lock                |
|KEY&#178;    |USBKEY_LOCKING_SCROLL_LOCK    |  0x84|Keyboard Locking Scroll Lock             |
|KEY&#178;    |USBKEY_KP_COMMA               |  0x85|Keypad Comma                             |
|KEY&#178;    |USBKEY_KP_EQUAL_SIGN          |  0x86|Keypad Equal Sign                        |
|KEY&#178;    |USBKEY_INT_1                  |  0x87|Keyboard International1                  |
|KEY&#178;    |USBKEY_INT_2                  |  0x88|Keyboard International2                  |
|KEY&#178;    |USBKEY_INT_3                  |  0x89|Keyboard International3                  |
|KEY&#178;    |USBKEY_INT_4                  |  0x8A|Keyboard International4                  |
|KEY&#178;    |USBKEY_INT_5                  |  0x8B|Keyboard International5                  |
|KEY&#178;    |USBKEY_INT_6                  |  0x8C|Keyboard International6                  |
|KEY&#178;    |USBKEY_INT_7                  |  0x8D|Keyboard International7                  |
|KEY&#178;    |USBKEY_INT_8                  |  0x8E|Keyboard International8                  |
|KEY&#178;    |USBKEY_INT_9                  |  0x8F|Keyboard International9                  |
|KEY&#178;    |USBKEY_IME_KANA               |  0x88|Keyboard International1                  |
|KEY&#178;    |USBKEY_IME_CONVERT            |  0x8A|Keyboard International2                  |
|KEY&#178;    |USBKEY_IME_NONCONVERT         |  0x8B|Keyboard International3                  |
|KEY&#178;    |USBKEY_LANG_1                 |  0x90|Keyboard LANG1                           |
|KEY&#178;    |USBKEY_LANG_2                 |  0x91|Keyboard LANG2                           |
|KEY&#178;    |USBKEY_LANG_3                 |  0x92|Keyboard LANG3                           |
|KEY&#178;    |USBKEY_LANG_4                 |  0x93|Keyboard LANG4                           |
|KEY&#178;    |USBKEY_LANG_5                 |  0x94|Keyboard LANG5                           |
|KEY&#178;    |USBKEY_LANG_6                 |  0x95|Keyboard LANG6                           |
|KEY&#178;    |USBKEY_LANG_7                 |  0x96|Keyboard LANG7                           |
|KEY&#178;    |USBKEY_LANG_8                 |  0x97|Keyboard LANG8                           |
|KEY&#178;    |USBKEY_LANG_9                 |  0x98|Keyboard LANG9                           |
|KEY&#178;    |USBKEY_ALT_ERASE              |  0x99|Keyboard Alternate Erase                 |
|KEY&#178;    |USBKEY_ATTN                   |  0x9A|Keyboard SysReq/Attention                |
|KEY&#178;    |USBKEY_CANCEL                 |  0x9B|Keyboard Cancel                          |
|KEY&#178;    |USBKEY_CLEAR                  |  0x9C|Keyboard Clear                           |
|KEY&#178;    |USBKEY_PRIOR                  |  0x9D|Keyboard Prior                           |
|KEY&#178;    |USBKEY_RETURN                 |  0x9E|Keyboard Return                          |
|KEY&#178;    |USBKEY_SEPARATOR              |  0x9F|Keyboard Separator                       |
|KEY&#178;    |USBKEY_OUT                    |  0xA0|Keyboard Out                             |
|KEY&#178;    |USBKEY_OPER                   |  0xA1|Keyboard Oper                            |
|KEY&#178;    |USBKEY_CLEAR_AGAIN            |  0xA2|Keyboard Clear/Again                     |
|KEY&#178;    |USBKEY_CRSEL_PROPS            |  0xA3|Keyboard CrSel/Props                     |
|KEY&#178;    |USBKEY_EXSEL                  |  0xA4|Keyboard ExSel                           |
|KEY&#178;    |USBKEY_KP_00                  |  0xB0|Keypad 00                                |
|KEY&#178;    |USBKEY_KP_000                 |  0xB1|Keypad 000                               |
|KEY&#178;    |USBKEY_THOUSENDS_SEP          |  0xB2|Thousands Separator                      |
|KEY&#178;    |USBKEY_DECIMAL_SEP            |  0xB3|Decimal Separator                        |
|KEY&#178;    |USBKEY_CURRENCY_UNIT          |  0xB4|Currency Unit                            |
|KEY&#178;    |USBKEY_CURRENCY_SUB_UNIT      |  0xB5|Currency Sub-unit                        |
|KEY&#178;    |USBKEY_KP_OPEN_BRACKET        |  0xB6|Keypad (                                 |
|KEY&#178;    |USBKEY_KP_CLOSE_BRACKET       |  0xB7|Keypad )                                 |
|KEY&#178;    |USBKEY_KP_OPEN_CURLY_BRACKET  |  0xB8|Keypad {                                 |
|KEY&#178;    |USBKEY_KP_CLOSE_CURLY_BRACKET |  0xB9|Keypad }                                 |
|KEY&#178;    |USBKEY_KP_TAB                 |  0xBA|Keypad Tab                               |
|KEY&#178;    |USBKEY_KP_BACKSPACE           |  0xBB|Keypad Backspace                         |
|KEY&#178;    |USBKEY_KP_A                   |  0xBC|Keypad A                                 |
|KEY&#178;    |USBKEY_KP_B                   |  0xBD|Keypad B                                 |
|KEY&#178;    |USBKEY_KP_C                   |  0xBE|Keypad C                                 |
|KEY&#178;    |USBKEY_KP_D                   |  0xBF|Keypad D                                 |
|KEY&#178;    |USBKEY_KP_E                   |  0xC0|Keypad E                                 |
|KEY&#178;    |USBKEY_KP_F                   |  0xC1|Keypad F                                 |
|KEY&#178;    |USBKEY_KP_XOR                 |  0xC2|Keypad XOR                               |
|KEY&#178;    |USBKEY_KP_CARET               |  0xC3|Keypad ^                                 |
|KEY&#178;    |USBKEY_KP_PERCENT             |  0xC4|Keypad %                                 |
|KEY&#178;    |USBKEY_KP_LESS                |  0xC5|Keypad <                                 |
|KEY&#178;    |USBKEY_KP_GREATER             |  0xC6|Keypad >                                 |
|KEY&#178;    |USBKEY_KP_AND                 |  0xC7|Keypad &                                 |
|KEY&#178;    |USBKEY_KP_AND2                |  0xC8|Keypad &&                                |
|KEY&#178;    |USBKEY_KP_OR                  |  0xC9|Keypad &#124;                            |
|KEY&#178;    |USBKEY_KP_OR2                 |  0xCA|Keypad &#124;&#124;                      |
|KEY&#178;    |USBKEY_KP_COLON               |  0xCB|Keypad :                                 |
|KEY&#178;    |USBKEY_KP_HASH                |  0xCC|Keypad #                                 |
|KEY&#178;    |USBKEY_KP_SPACE               |  0xCD|Keypad Space                             |
|KEY&#178;    |USBKEY_KP_AT                  |  0xCE|Keypad @                                 |
|KEY&#178;    |USBKEY_KP_EXCLAMATION         |  0xCF|Keypad !                                 |
|KEY&#178;    |USBKEY_KP_MEM_STORE           |  0xD0|Keypad Memory Store                      |
|KEY&#178;    |USBKEY_KP_MEM_RECALL          |  0xD1|Keypad Memory Recall                     |
|KEY&#178;    |USBKEY_KP_MEM_CLEAR           |  0xD2|Keypad Memory Clear                      |
|KEY&#178;    |USBKEY_KP_MEM_ADD             |  0xD3|Keypad Memory Add                        |
|KEY&#178;    |USBKEY_KP_MEM_SUB             |  0xD4|Keypad Memory Subtract                   |
|KEY&#178;    |USBKEY_KP_MEM_MUL             |  0xD5|Keypad Memory Multiply                   |
|KEY&#178;    |USBKEY_KP_MEM_DIV             |  0xD6|Keypad Memory Divide                     |
|KEY&#178;    |USBKEY_KP_PLUS_MINUS          |  0xD7|Keypad +/-                               |
|KEY&#178;    |USBKEY_KP_CLEAR               |  0xD8|Keypad Clear                             |
|KEY&#178;    |USBKEY_KP_CLEAR_ENTRY         |  0xD9|Keypad Clear Entry                       |
|KEY&#178;    |USBKEY_KP_BIN                 |  0xDA|Keypad Binary                            |
|KEY&#178;    |USBKEY_KP_OCT                 |  0xDB|Keypad Octal                             |
|KEY&#178;    |USBKEY_KP_DEC                 |  0xDC|Keypad Decimal                           |
|KEY&#178;    |USBKEY_KP_HEX                 |  0xDD|Keypad Hexadecimal                       |
|KEY&#178;    |USBKEY_LEFT_CONTROL           |  0xE0|Keyboard LeftControl                     |
|KEY&#178;    |USBKEY_LEFT_SHIFT             |  0xE1|Keyboard LeftShift                       |
|KEY&#178;    |USBKEY_LEFT_ALT               |  0xE2|Keyboard LeftAlt                         |
|KEY&#178;    |USBKEY_LEFT_GUI               |  0xE3|Keyboard Left GUI                        |
|KEY&#178;    |USBKEY_RIGHT_CONTROL          |  0xE4|Keyboard RightControl                    |
|KEY&#178;    |USBKEY_RIGHT_SHIFT            |  0xE5|Keyboard RightShift                      |
|KEY&#178;    |USBKEY_RIGHT_ALT              |  0xE6|Keyboard RightAlt                        |
|KEY&#178;    |USBKEY_RIGHT_GUI              |  0xE7|Keyboard Right GUI                       |
|MOD          |USBWRITE_LEFT_CONTROL         |  0x01|Left control key is being pressed.       |
|MOD          |USBWRITE_LEFT_SHIFT           |  0x02|Left shift key is being pressed.         |
|MOD          |USBWRITE_LEFT_ALT             |  0x04|Left alt key is being pressed.           |
|MOD          |USBWRITE_RIGHT_CONTROL        |  0x08|Right control key is being pressed.      |
|MOD          |USBWRITE_RIGHT_SHIFT          |  0x10|Right shift key is being pressed.        |
|MOD          |USBWRITE_RIGHT_ALT            |  0x20|Right alt key is being pressed.          |
|MOD          |USBWRITE_RIGHT_NUM_LOCK       |  0x40|Num lock is active.                      |
|MOD          |USBWRITE_RIGHT_KANA           |  0x80|Kana is being enabled.                   |
|BUTTON&#179; |USBBUTTON_LEFT                |  0x01|Button 1 (primary/trigger)               |
|BUTTON&#179; |USBBUTTON_RIGHT               |  0x02|Button 2 (secondary)                     |
|BUTTON&#179; |USBBUTTON_MIDDLE              |  0x04|Button 3 (tertiary)                      |

1\) See also [HID Usage Tables for USB v1.2 ch. 11](https://www.usb.org/sites/default/files/hut1_2.pdf#page=91) for LED values.  
2\) See also [HID Usage Tables for USB v1.2 ch. 10](https://www.usb.org/sites/default/files/hut1_2.pdf#page=83) for keyboard key values.  
3\) See also [HID Usage Tables for USB v1.2 ch. 12](https://www.usb.org/sites/default/files/hut1_2.pdf#page=103) for mouse button values.  

### Request/Response Fields

|Size |C Type  |Name    |Description                  |
|----:|-------:|--------|-----------------------------|
|    1| uint8_t|KEY     |Keyboard key values. Up to 6 |
|    1| uint8_t|NKEY    |Number of keys processed.    |
|    1| uint8_t|MOD     |Keyboard key modifier.       |
|    1| uint8_t|USB     |USB periphery state.         |
|    1| uint8_t|LED     |Keyboard LED bits.           |
|    1| uint8_t|BUTTON  |Mouse button value.          |
|    1| uint8_t|NBUTTON |Number of buttons processed. |
|    1|  int8_t|REL_X   |Relative mouse x coordinate. |
|    1|  int8_t|REL_Y   |Relative mouse y coordinate. |
|    1|  int8_t|WHEEL   |Relative mouse wheel change. |
|    2| int16_t|ABS_X   |Absolute mouse x coordinate. |
|    2| int16_t|ABS_Y   |Absolute mouse y coordinate. |
|    2|uint16_t|VER     |Protocol version (0x0100).   |

### Request Message

The user payload is structured as following for requests (control &#8680; periphery).

|Byte |Size |Field |Description               |
|----:|----:|------|--------------------------|
|    0|    1|REQ   |Request type.             |
|    1|  N-1|FIELDS|Request fields (optional).|

### Response Message

The user payload is structured as following for responses (control &#8678; periphery).

|Byte |Size |Field |Description                |
|----:|----:|------|---------------------------|
|    0|    1|RES   |Response type.             |
|    1|  N-1|FIELDS|Response fields (optional).|

### Request/Response Description

The following request/response messages are defined including unsolicited responses.

|Name                            |Request Fields       |Response Fields   |
|--------------------------------|---------------------|------------------|
|GET_PROTOCOL_VERSION            |-                    |VER               |
|GET_ALIVE                       |-                    |-                 |
|GET_USB_STATE                   |-                    |USB               |
|GET_KEYBOARD_LEDS               |-                    |LED               |
|SET_KEYBOARD_DOWN               |KEY\[1..6]           |-                 |
|SET_KEYBOARD_UP                 |KEY\[1..6]           |-                 |
|SET_KEYBOARD_ALL_UP             |-                    |-                 |
|SET_KEYBOARD_PUSH               |KEY\[0..N]&#185;     |NKEY              |
|SET_KEYBOARD_WRITE              |MOD, KEY\[0..N]&#185;|NKEY              |
|SET_MOUSE_BUTTON_DOWN           |BUTTON\[1..3]        |-                 |
|SET_MOUSE_BUTTON_UP             |BUTTON\[1..3]        |-                 |
|SET_MOUSE_BUTTON_ALL_UP         |-                    |-                 |
|SET_MOUSE_BUTTON_PUSH           |BUTTON\[0..N]&#185;  |NBUTTON           |
|SET_MOUSE_MOVE_ABS              |ABS_X, ABS_Y         |-                 |
|SET_MOUSE_MOVE_REL              |REL_X, REL_Y         |-                 |
|SET_MOUSE_SCROLL                |WHEEL                |-                 |
|USB state update interrupt&#178;|-                    |USB               |
|LED update interrupt&#179;      |-                    |LED               |
|DEBUG message &#8308;           |-                    |<arbitrary string>|

1\) The maximum frame size is 256 bytes which limits the maximum number of keys/buttons per request.  
2\) The USB state update interrupt has the sequence number 0 and response type I_USB_STATE_UPDATE.  
3\) The LED update interrupt has the sequence number 0 and response type I_LED_UPDATE.  
4\) The DEBUG message has the sequence number 0 and response type D_MESSAGE.
