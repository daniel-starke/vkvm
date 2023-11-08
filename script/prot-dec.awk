#!/usr/bin/gawk -f
# @file prot-dec.awk
# @author Daniel Starke
# @date 2022-08-05
# @version 2023-10-04
#
# VKVM (src/pcf/serial/Vkvm.cpp) trace decoder.

# print a single frame field value
function printValue(value, size, fields, idx,   subIdx, field, i, first) {
	if (idx > 0xFF) {
		subIdx = rshift(idx, 8);
		idx = and(idx, 0xFF);
	} else {
		subIdx = idx;
	}
	field = fields[subIdx];
	if (substr(field, length(field), 1) == "#") {
		# value name with index
		field = sprintf("%s%i", substr(field, 1, length(field) - 1), idx - subIdx);
		idx = lshift(subIdx, 8) + idx + 1;
	} else {
		idx++;
	}
	# field type specific output
	if (field ~ /^[\$-]?usb$/ && value in USBSTATE) {
		# print USB periphery state
		gsub(/^[\$-]/, "", field);
		printf("\t%s:<%s>", field, USBSTATE[value]);
	} else if (field ~ /^[\$-]?key[0-9]+$/ && value in USBKEY) {
		# print key
		gsub(/^[\$-]/, "", field);
		printf("\t%s:<%s>", field, USBKEY[value]);
	} else if (field ~ /^[\$-]?leds$/) {
		# print led bit field
		gsub(/^[\$-]/, "", field);
		printf("\t%s:", field);
		if (value == 0) {
			printf("<none>");
		} else {
			first = 1;
			for (i in USBLED) {
				if (and(value, i) != 0) {
					if (first != 0) {
						first = 0;
					} else {
						printf("|");
					}
					printf("<%s>", USBLED[i]);
				}
			}
		}
	} else if (field ~ /^msg[0-9]+$/) {
		# print debug message
		if (and(idx, 0xFF) == 2) {
			printf("\tmsg:");
		}
		printf("%c", value);
	} else if (substr(field, 1, 1) == "$") {
		# print hex
		printf("\t%s:0x%0" size "X", substr(field, 2), value);
	} else if (substr(field, 1, 1) == "-") {
		# print signed dec
		if (size == 4) {
			printf("\t%s:%i", substr(field, 2), (value < 32768) ? value : -(xor(value, 0xFFFF) + 1));
		} else {
			printf("\t%s:%i", substr(field, 2), (value < 128) ? value : -(xor(value, 0xFF) + 1));
		}
	} else {
		# print unsigned dec
		printf("\t%s:%i", field, value);
	}
	return idx;
}

# print all frame field values
function printValues(frame, fields, inWords,   fieldIdx, value, i) {
	fieldIdx = 1;
	if (inWords == 1) {
		for (i = 3; (i + 1) <= length(frame); i += 2) {
			value = frame[i] * 256 + frame[i + 1];
			fieldIdx = printValue(value, 4, fields, fieldIdx);
		}
	} else {
		for (i = 3; i <= length(frame); i++) {
			value = frame[i];
			fieldIdx = printValue(value, 2, fields, fieldIdx);
		}
	}
}

# print the content of a request frame
function processRequest(time, dir, frame,   name) {
	if (length(frame) == 0) return;
	printf("%s\t%s\tseq:%i", time, dir, frame[1]);
	if (length(frame) > 1) {
		if (frame[2] in REQ) {
			split(REQ[frame[2]], name, ",");
			R[frame[1]] = frame[2]; # remember request type for this sequence number
			printf("\ttype:%s", name[1]);
			split(name[2], fields, ";");
			printValues(frame, fields, REQw[frame[2]]);
		} else {
			printf("\ttype:<ERROR:unknown request 0x%02X>", frame[2]);
		}
	}
	printf("\n")
}

# print the content of a response frame
function processResponse(time, dir, frame,   name, fields) {
	if (length(frame) == 0) return;
	printf("%s\t%s\tseq:%i", time, dir, frame[1]);
	if (length(frame) > 1) {
		if (frame[2] in RES) {
			split(RES[frame[2]], name, ",");
			printf("\ttype:%s", name[1]);
			if (frame[1] in R || frame[1] == 0) {
				if (frame[1] == 0) {
					# interrupt
					split(name[2], fields, ";");
				} else {
					# response
					split(REQ[R[frame[1]]], name, ",");
					split(name[3], fields, ";");
				}
				printValues(frame, fields, REQw[R[frame[1]]]);
			} else {
				printf("\tvalues:<ERROR:missing request>");
			}
		} else {
			printf("\ttype:<ERROR:unknown response>");
		}
	}
	printf("\n")
}

# process the raw data: unframe -> unquote -> remove CRC -> print
function processData(array, time, dir, data, isReq, esc,   n, byte, arr) {
	n = split(data, arr, " ");
	for (i = 1; i <= n; i++) {
		byte = strtonum("0x" arr[i]);
		if (esc != 0) {
			array[length(array) + 1] = xor(byte, 0x20);
			esc = 0;
		} else {
			if (byte == 0x7D) {
				esc = 1;
			} else if (byte == 0x7E) {
				# remove CRC
				delete array[length(array)];
				delete array[length(array)];
				# process payload
				if (isReq == 1) {
					processRequest(time, dir, array);
				} else {
					processResponse(time, dir, array);
				}
				delete array;
			} else {
				array[length(array) + 1] = byte;
			}
		}
	}
	return esc;
}

BEGIN {
	FS = "[\t\r\n]";
	# requests
	REQ[0x00] = "GET_PROTOCOL_VERSION,,$version";
	REQ[0x01] = "GET_ALIVE,,";
	REQ[0x02] = "GET_USB_STATE,,$usb";
	REQ[0x03] = "GET_KEYBOARD_LEDS,,$leds";
	REQ[0x04] = "SET_KEYBOARD_DOWN,$key#,$keyMap";
	REQ[0x05] = "SET_KEYBOARD_UP,$key#,$keyMap";
	REQ[0x06] = "SET_KEYBOARD_ALL_UP,,";
	REQ[0x07] = "SET_KEYBOARD_PUSH,$key#,written";
	REQ[0x08] = "SET_KEYBOARD_WRITE,mod;$key#,written";
	REQ[0x09] = "SET_MOUSE_BUTTON_DOWN,button#,";
	REQ[0x0A] = "SET_MOUSE_BUTTON_UP,button#,";
	REQ[0x0B] = "SET_MOUSE_BUTTON_ALL_UP,,";
	REQ[0x0C] = "SET_MOUSE_BUTTON_PUSH,button#,written";
	REQ[0x0D] = "SET_MOUSE_MOVE_ABS,-x;-y,";
	REQ[0x0E] = "SET_MOUSE_MOVE_REL,-x;-y,";
	REQ[0x0F] = "SET_MOUSE_SCROLL,-wheel,";
	# responses
	RES[0x00] = "S_OK";
	RES[0x40] = "I_USB_STATE_UPDATE,$usb";
	RES[0x41] = "I_LED_UPDATE,$leds";
	RES[0x60] = "D_MESSAGE,msg#";
	RES[0x80] = "E_BROKEN_FRAME";
	RES[0x81] = "E_UNSUPPORTED_REQ_TYPE";
	RES[0x82] = "E_INVALID_REQ_TYPE";
	RES[0x83] = "E_INVALID_FIELD_VALUE,index";
	RES[0x85] = "E_HOST_WRITE_ERROR";
	# value size == 2 byte
	REQw[0x00] = 1;
	REQw[0x0D] = 1;
	# USB state
	USBSTATE[0x00] = "OFF";
	USBSTATE[0x01] = "ON";
	USBSTATE[0x02] = "CONFIGURED";
	USBSTATE[0x03] = "ON_CONFIGURED";
	# keys
	USBKEY[0x00] = "NO_EVENT";
	USBKEY[0x01] = "ERROR_ROLL_OVER";
	USBKEY[0x02] = "POST_FAIL";
	USBKEY[0x03] = "ERROR_UNDEFINED";
	USBKEY[0x04] = "A";
	USBKEY[0x05] = "B";
	USBKEY[0x06] = "C";
	USBKEY[0x07] = "D";
	USBKEY[0x08] = "E";
	USBKEY[0x09] = "F";
	USBKEY[0x0A] = "G";
	USBKEY[0x0B] = "H";
	USBKEY[0x0C] = "I";
	USBKEY[0x0D] = "J";
	USBKEY[0x0E] = "K";
	USBKEY[0x0F] = "L";
	USBKEY[0x10] = "M";
	USBKEY[0x11] = "N";
	USBKEY[0x12] = "O";
	USBKEY[0x13] = "P";
	USBKEY[0x14] = "Q";
	USBKEY[0x15] = "R";
	USBKEY[0x16] = "S";
	USBKEY[0x17] = "T";
	USBKEY[0x18] = "U";
	USBKEY[0x19] = "V";
	USBKEY[0x1A] = "W";
	USBKEY[0x1B] = "X";
	USBKEY[0x1C] = "Y";
	USBKEY[0x1D] = "Z";
	USBKEY[0x1E] = "1";
	USBKEY[0x1F] = "2";
	USBKEY[0x20] = "3";
	USBKEY[0x21] = "4";
	USBKEY[0x22] = "5";
	USBKEY[0x23] = "6";
	USBKEY[0x24] = "7";
	USBKEY[0x25] = "8";
	USBKEY[0x26] = "9";
	USBKEY[0x27] = "0";
	USBKEY[0x28] = "ENTER";
	USBKEY[0x29] = "ESCAPE";
	USBKEY[0x2A] = "BACKSPACE";
	USBKEY[0x2B] = "TAB";
	USBKEY[0x2C] = "SPACE";
	USBKEY[0x2D] = "MINUS";
	USBKEY[0x2E] = "EQUAL";
	USBKEY[0x2F] = "OPEN_BRACKET";
	USBKEY[0x30] = "CLOSE_BRACKET";
	USBKEY[0x31] = "BACKSLASH";
	USBKEY[0x32] = "NON_US_HASH";
	USBKEY[0x33] = "SEMICOLON";
	USBKEY[0x34] = "APOSTROPHE";
	USBKEY[0x35] = "ACCENT";
	USBKEY[0x36] = "COMMA";
	USBKEY[0x37] = "PERIOD";
	USBKEY[0x38] = "SLASH";
	USBKEY[0x39] = "CAPS_LOCK";
	USBKEY[0x3A] = "F1";
	USBKEY[0x3B] = "F2";
	USBKEY[0x3C] = "F3";
	USBKEY[0x3D] = "F4";
	USBKEY[0x3E] = "F5";
	USBKEY[0x3F] = "F6";
	USBKEY[0x40] = "F7";
	USBKEY[0x41] = "F8";
	USBKEY[0x42] = "F9";
	USBKEY[0x43] = "F10";
	USBKEY[0x44] = "F11";
	USBKEY[0x45] = "F12";
	USBKEY[0x46] = "PRINT_SCREEN";
	USBKEY[0x47] = "SCROLL_LOCK";
	USBKEY[0x48] = "PAUSE";
	USBKEY[0x49] = "INSERT";
	USBKEY[0x4A] = "HOME";
	USBKEY[0x4B] = "PAGE_UP";
	USBKEY[0x4C] = "DELETE";
	USBKEY[0x4D] = "END";
	USBKEY[0x4E] = "PAGE_DOWN";
	USBKEY[0x4F] = "RIGHT_ARROW";
	USBKEY[0x50] = "LEFT_ARROW";
	USBKEY[0x51] = "DOWN_ARROW";
	USBKEY[0x52] = "UP_ARROW";
	USBKEY[0x53] = "NUM_LOCK";
	USBKEY[0x54] = "KP_DIVIDE";
	USBKEY[0x55] = "KP_MULTIPLY";
	USBKEY[0x56] = "KP_SUBTRACT";
	USBKEY[0x57] = "KP_ADD";
	USBKEY[0x58] = "KP_ENTER";
	USBKEY[0x59] = "KP_1";
	USBKEY[0x5A] = "KP_2";
	USBKEY[0x5B] = "KP_3";
	USBKEY[0x5C] = "KP_4";
	USBKEY[0x5D] = "KP_5";
	USBKEY[0x5E] = "KP_6";
	USBKEY[0x5F] = "KP_7";
	USBKEY[0x60] = "KP_8";
	USBKEY[0x61] = "KP_9";
	USBKEY[0x62] = "KP_0";
	USBKEY[0x63] = "KP_DECIMAL";
	USBKEY[0x64] = "NON_US_BACKSLASH";
	USBKEY[0x65] = "APPLICATION";
	USBKEY[0x66] = "POWER";
	USBKEY[0x67] = "KP_EQUAL";
	USBKEY[0x68] = "F13";
	USBKEY[0x69] = "F14";
	USBKEY[0x6A] = "F15";
	USBKEY[0x6B] = "F16";
	USBKEY[0x6C] = "F17";
	USBKEY[0x6D] = "F18";
	USBKEY[0x6E] = "F19";
	USBKEY[0x6F] = "F20";
	USBKEY[0x70] = "F21";
	USBKEY[0x71] = "F22";
	USBKEY[0x72] = "F23";
	USBKEY[0x73] = "F24";
	USBKEY[0x74] = "EXECUTE";
	USBKEY[0x75] = "HELP";
	USBKEY[0x76] = "MENU";
	USBKEY[0x77] = "SELECT";
	USBKEY[0x78] = "STOP";
	USBKEY[0x79] = "AGAIN";
	USBKEY[0x7A] = "UNDO";
	USBKEY[0x7B] = "CUT";
	USBKEY[0x7C] = "COPY";
	USBKEY[0x7D] = "PASTE";
	USBKEY[0x7E] = "FIND";
	USBKEY[0x7F] = "MUTE";
	USBKEY[0x80] = "VOLUME_UP";
	USBKEY[0x81] = "VOLUME_DOWN";
	USBKEY[0x82] = "LOCKING_CAPS_LOCK";
	USBKEY[0x83] = "LOCKING_NUM_LOCK";
	USBKEY[0x84] = "LOCKING_SCROLL_LOCK";
	USBKEY[0x85] = "KP_COMMA";
	USBKEY[0x86] = "KP_EQUAL_SIGN";
	USBKEY[0x87] = "INT_1";
	USBKEY[0x88] = "INT_2";
	USBKEY[0x89] = "INT_3";
	USBKEY[0x8A] = "INT_4";
	USBKEY[0x8B] = "INT_5";
	USBKEY[0x8C] = "INT_6";
	USBKEY[0x8D] = "INT_7";
	USBKEY[0x8E] = "INT_8";
	USBKEY[0x8F] = "INT_9";
	USBKEY[0x88] = "IME_KANA";
	USBKEY[0x8A] = "IME_CONVERT";
	USBKEY[0x8B] = "IME_NONCONVERT";
	USBKEY[0x90] = "LANG_1";
	USBKEY[0x91] = "LANG_2";
	USBKEY[0x92] = "LANG_3";
	USBKEY[0x93] = "LANG_4";
	USBKEY[0x94] = "LANG_5";
	USBKEY[0x95] = "LANG_6";
	USBKEY[0x96] = "LANG_7";
	USBKEY[0x97] = "LANG_8";
	USBKEY[0x98] = "LANG_9";
	USBKEY[0x99] = "ALT_ERASE";
	USBKEY[0x9A] = "ATTN";
	USBKEY[0x9B] = "CANCEL";
	USBKEY[0x9C] = "CLEAR";
	USBKEY[0x9D] = "PRIOR";
	USBKEY[0x9E] = "RETURN";
	USBKEY[0x9F] = "SEPARATOR";
	USBKEY[0xA0] = "OUT";
	USBKEY[0xA1] = "OPER";
	USBKEY[0xA2] = "CLEAR_AGAIN";
	USBKEY[0xA3] = "CRSEL_PROPS";
	USBKEY[0xA4] = "EXSEL";
	USBKEY[0xB0] = "KP_00";
	USBKEY[0xB1] = "KP_000";
	USBKEY[0xB2] = "THOUSENDS_SEP";
	USBKEY[0xB3] = "DECIMAL_SEP";
	USBKEY[0xB4] = "CURRENCY_UNIT";
	USBKEY[0xB5] = "CURRENCY_SUB_UNIT";
	USBKEY[0xB6] = "KP_OPEN_BRACKET";
	USBKEY[0xB7] = "KP_CLOSE_BRACKET";
	USBKEY[0xB8] = "KP_OPEN_CURLY_BRACKET";
	USBKEY[0xB9] = "KP_CLOSE_CURLY_BRACKET";
	USBKEY[0xBA] = "KP_TAB";
	USBKEY[0xBB] = "KP_BACKSPACE";
	USBKEY[0xBC] = "KP_A";
	USBKEY[0xBD] = "KP_B";
	USBKEY[0xBE] = "KP_C";
	USBKEY[0xBF] = "KP_D";
	USBKEY[0xC0] = "KP_E";
	USBKEY[0xC1] = "KP_F";
	USBKEY[0xC2] = "KP_XOR";
	USBKEY[0xC3] = "KP_CARET";
	USBKEY[0xC4] = "KP_PERCENT";
	USBKEY[0xC5] = "KP_LESS";
	USBKEY[0xC6] = "KP_GREATER";
	USBKEY[0xC7] = "KP_AND";
	USBKEY[0xC8] = "KP_AND2";
	USBKEY[0xC9] = "KP_OR";
	USBKEY[0xCA] = "KP_OR2";
	USBKEY[0xCB] = "KP_COLON";
	USBKEY[0xCC] = "KP_HASH";
	USBKEY[0xCD] = "KP_SPACE";
	USBKEY[0xCE] = "KP_AT";
	USBKEY[0xCF] = "KP_EXCLAMATION";
	USBKEY[0xD0] = "KP_MEM_STORE";
	USBKEY[0xD1] = "KP_MEM_RECALL";
	USBKEY[0xD2] = "KP_MEM_CLEAR";
	USBKEY[0xD3] = "KP_MEM_ADD";
	USBKEY[0xD4] = "KP_MEM_SUB";
	USBKEY[0xD5] = "KP_MEM_MUL";
	USBKEY[0xD6] = "KP_MEM_DIV";
	USBKEY[0xD7] = "KP_PLUS_MINUS";
	USBKEY[0xD8] = "KP_CLEAR";
	USBKEY[0xD9] = "KP_CLEAR_ENTRY";
	USBKEY[0xDA] = "KP_BIN";
	USBKEY[0xDB] = "KP_OCT";
	USBKEY[0xDC] = "KP_DEC";
	USBKEY[0xDD] = "KP_HEX";
	USBKEY[0xE0] = "LEFT_CONTROL";
	USBKEY[0xE1] = "LEFT_SHIFT";
	USBKEY[0xE2] = "LEFT_ALT";
	USBKEY[0xE3] = "LEFT_GUI";
	USBKEY[0xE4] = "RIGHT_CONTROL";
	USBKEY[0xE5] = "RIGHT_SHIFT";
	USBKEY[0xE6] = "RIGHT_ALT";
	USBKEY[0xE7] = "RIGHT_GUI";
	# leds
	USBLED[0x01] = "NUM_LOCK";
	USBLED[0x02] = "CAPS_LOCK";
	USBLED[0x04] = "SCROLL_LOCK";
	USBLED[0x08] = "COMPOSE";
	USBLED[0x10] = "KANA";
	USBLED[0x20] = "POWER";
	USBLED[0x40] = "SHIFT";
	USBLED[0x80] = "DO_NOT_DISTURB";
	# disconnect reasons
	DISC[0] = "D_USER";
	DISC[1] = "D_RECV_ERROR";
	DISC[2] = "D_SEND_ERROR";
	DISC[3] = "D_INVALID_PROTOCOL";
	DISC[4] = "D_TIMEOUT";
	# global variables
	reqEsc = 0;
	resEsc = 0;
	delete reqData;
	delete resData;
}

/^[0-9]+\t/ {
	gsub(/\r/, "");
	if ($2 == "in") {
		reqEsc = processData(reqData, $1, $2, $3, 0, reqEsc);
	} else if ($2 == "out") {
		resEsc = processData(resData, $1, $2, $3, 1, resEsc);
	} else if ($2 == "invalid") {
		printf("%s\t%s\tseq:%s\n", $1, $2, $3);
	} else if (($2 == "disconnect" || $2 == "disconnect forwarded") && $3 in DISC) {
		printf("%s\t%s\treason:%s\n", $1, $2, DISC[$3]);
	} else if ($2 == "timeout1" || $2 == "timeout2") {
		printf("%s\t%s\t%ims\n", $1, $2, $1 - $3);
	} else {
		printf("%s\n", $0);
	}
}
