/**
 * @file keyTest.cpp
 * @author Daniel Starke
 * @date 2019-11-03
 * @version 2023-10-08
 */
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <libpcf/target.h>
#include <pcf/serial/Vkvm.hpp>
#ifndef PCF_IS_WIN
extern "C" {
#include <linux/input.h>
}
#endif /* PCF_IS_WIN */


#ifndef KEY_CNT
#define KEY_CNT 0x100
#endif /* KEY_CNT */



/**
 * Helper class to perform the key test iteration and response output.
 * The result is written to stdout. Errors are reported via stderr.
 */
class KeyTest : public pcf::serial::VkvmCallback {
private:
	struct MapItem {
		uint8_t usb;
		int os;
	};
	pcf::serial::VkvmDevice device; /**< VKVM device instance. */
	std::mutex running; /**< Single instance running mutex. */
	std::thread worker; /**< Command worker thread. */
	uint8_t curUsb; /**< USB HID key code currently tested. */
	MapItem * mapping; /**< USB HID to OS key map */
public:
	/**
	 * Constructor.
	 */
	explicit KeyTest():
		mapping(static_cast<MapItem *>(malloc(sizeof(MapItem) * KEY_CNT)))
	{
		if (this->mapping == NULL) throw std::bad_alloc();
	}

	/**
	 * Destructor.
	 */
	~KeyTest() {
		try {
			this->join();
			if ( this->worker.joinable() ) fprintf(stderr, "Error: Working thread is still joinable.\n");
		} catch (const std::exception & e) {
			fprintf(stderr, "Error: %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "Error: Caught unknown exception in KeyTest destructor.\n");
		}
		if (this->mapping != NULL) free(this->mapping);
	}

	/**
	 * Starts
	 *
	 * @param[in] devicePath - path to the serial connected VKVM device
	 * @return true on success, else false
	 */
	bool start(const char * devicePath) {
		if ( ! this->running.try_lock() ) return false;
		if ( ! this->device.open(*this, devicePath) ) {
			this->running.unlock();
			return false;
		}
		return true;
	}

	/**
	 * Waits for all operations to finish.
	 */
	void join() {
		if ( this->running.try_lock() ) {
			if ( this->worker.joinable() ) this->worker.join();
			this->running.unlock();
			return; /* not running */
		}
		/* wait for device to close / finish the test */
		this->running.lock();
		if ( this->worker.joinable() ) this->worker.join();
		this->device.close();
		this->running.unlock();
		return;
	}
private:
	/**
	 * Handles the completed VKVM device connection event.
	 */
	virtual void onVkvmConnected() {
		if ( ! this->device.grabGlobalInput(true) ) {
			fprintf(stderr, "Error: Failed to start input capture. Insufficient permissions?\n");
			this->device.close();
			return;
		}
		/* run command execution thread */
		this->worker = std::thread([this] () {
			static const char * usbKeyStr[] = {
				"USBKEY_NO_EVENT",
				"USBKEY_ERROR_ROLL_OVER",
				"USBKEY_POST_FAIL",
				"USBKEY_ERROR_UNDEFINED",
				"USBKEY_A",
				"USBKEY_B",
				"USBKEY_C",
				"USBKEY_D",
				"USBKEY_E",
				"USBKEY_F",
				"USBKEY_G",
				"USBKEY_H",
				"USBKEY_I",
				"USBKEY_J",
				"USBKEY_K",
				"USBKEY_L",
				"USBKEY_M",
				"USBKEY_N",
				"USBKEY_O",
				"USBKEY_P",
				"USBKEY_Q",
				"USBKEY_R",
				"USBKEY_S",
				"USBKEY_T",
				"USBKEY_U",
				"USBKEY_V",
				"USBKEY_W",
				"USBKEY_X",
				"USBKEY_Y",
				"USBKEY_Z",
				"USBKEY_1",
				"USBKEY_2",
				"USBKEY_3",
				"USBKEY_4",
				"USBKEY_5",
				"USBKEY_6",
				"USBKEY_7",
				"USBKEY_8",
				"USBKEY_9",
				"USBKEY_0",
				"USBKEY_ENTER",
				"USBKEY_ESCAPE",
				"USBKEY_BACKSPACE",
				"USBKEY_TAB",
				"USBKEY_SPACE",
				"USBKEY_MINUS",
				"USBKEY_EQUAL",
				"USBKEY_OPEN_BRACKET",
				"USBKEY_CLOSE_BRACKET",
				"USBKEY_BACKSLASH",
				"USBKEY_NON_US_HASH",
				"USBKEY_SEMICOLON",
				"USBKEY_APOSTROPHE",
				"USBKEY_ACCENT",
				"USBKEY_COMMA",
				"USBKEY_PERIOD",
				"USBKEY_SLASH",
				"USBKEY_CAPS_LOCK",
				"USBKEY_F1",
				"USBKEY_F2",
				"USBKEY_F3",
				"USBKEY_F4",
				"USBKEY_F5",
				"USBKEY_F6",
				"USBKEY_F7",
				"USBKEY_F8",
				"USBKEY_F9",
				"USBKEY_F10",
				"USBKEY_F11",
				"USBKEY_F12",
				"USBKEY_PRINT_SCREEN",
				"USBKEY_SCROLL_LOCK",
				"USBKEY_PAUSE",
				"USBKEY_INSERT",
				"USBKEY_HOME",
				"USBKEY_PAGE_UP",
				"USBKEY_DELETE",
				"USBKEY_END",
				"USBKEY_PAGE_DOWN",
				"USBKEY_RIGHT_ARROW",
				"USBKEY_LEFT_ARROW",
				"USBKEY_DOWN_ARROW",
				"USBKEY_UP_ARROW",
				"USBKEY_NUM_LOCK",
				"USBKEY_KP_DIVIDE",
				"USBKEY_KP_MULTIPLY",
				"USBKEY_KP_SUBTRACT",
				"USBKEY_KP_ADD",
				"USBKEY_KP_ENTER",
				"USBKEY_KP_1",
				"USBKEY_KP_2",
				"USBKEY_KP_3",
				"USBKEY_KP_4",
				"USBKEY_KP_5",
				"USBKEY_KP_6",
				"USBKEY_KP_7",
				"USBKEY_KP_8",
				"USBKEY_KP_9",
				"USBKEY_KP_0",
				"USBKEY_KP_DECIMAL",
				"USBKEY_NON_US_BACKSLASH",
				"USBKEY_APPLICATION",
				"USBKEY_POWER",
				"USBKEY_KP_EQUAL",
				"USBKEY_F13",
				"USBKEY_F14",
				"USBKEY_F15",
				"USBKEY_F16",
				"USBKEY_F17",
				"USBKEY_F18",
				"USBKEY_F19",
				"USBKEY_F20",
				"USBKEY_F21",
				"USBKEY_F22",
				"USBKEY_F23",
				"USBKEY_F24",
				"USBKEY_EXECUTE",
				"USBKEY_HELP",
				"USBKEY_MENU",
				"USBKEY_SELECT",
				"USBKEY_STOP",
				"USBKEY_AGAIN",
				"USBKEY_UNDO",
				"USBKEY_CUT",
				"USBKEY_COPY",
				"USBKEY_PASTE",
				"USBKEY_FIND",
				"USBKEY_MUTE",
				"USBKEY_VOLUME_UP",
				"USBKEY_VOLUME_DOWN",
				"USBKEY_LOCKING_CAPS_LOCK",
				"USBKEY_LOCKING_NUM_LOCK",
				"USBKEY_LOCKING_SCROLL_LOCK",
				"USBKEY_KP_COMMA",
				"USBKEY_KP_EQUAL_SIGN",
				"USBKEY_INT_1",
				"USBKEY_INT_2",
				"USBKEY_INT_3",
				"USBKEY_INT_4",
				"USBKEY_INT_5",
				"USBKEY_INT_6",
				"USBKEY_INT_7",
				"USBKEY_INT_8",
				"USBKEY_INT_9",
				"USBKEY_LANG_1",
				"USBKEY_LANG_2",
				"USBKEY_LANG_3",
				"USBKEY_LANG_4",
				"USBKEY_LANG_5",
				"USBKEY_LANG_6",
				"USBKEY_LANG_7",
				"USBKEY_LANG_8",
				"USBKEY_LANG_9",
				"USBKEY_ALT_ERASE",
				"USBKEY_ATTN",
				"USBKEY_CANCEL",
				"USBKEY_CLEAR",
				"USBKEY_PRIOR",
				"USBKEY_RETURN",
				"USBKEY_SEPARATOR",
				"USBKEY_OUT",
				"USBKEY_OPER",
				"USBKEY_CLEAR_AGAIN",
				"USBKEY_CRSEL_PROPS",
				"USBKEY_EXSEL",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"USBKEY_KP_00",
				"USBKEY_KP_000",
				"USBKEY_THOUSENDS_SEP",
				"USBKEY_DECIMAL_SEP",
				"USBKEY_CURRENCY_UNIT",
				"USBKEY_CURRENCY_SUB_UNIT",
				"USBKEY_KP_OPEN_BRACKET",
				"USBKEY_KP_CLOSE_BRACKET",
				"USBKEY_KP_OPEN_CURLY_BRACKET",
				"USBKEY_KP_CLOSE_CURLY_BRACKET",
				"USBKEY_KP_TAB",
				"USBKEY_KP_BACKSPACE",
				"USBKEY_KP_A",
				"USBKEY_KP_B",
				"USBKEY_KP_C",
				"USBKEY_KP_D",
				"USBKEY_KP_E",
				"USBKEY_KP_F",
				"USBKEY_KP_XOR",
				"USBKEY_KP_CARET",
				"USBKEY_KP_PERCENT",
				"USBKEY_KP_LESS",
				"USBKEY_KP_GREATER",
				"USBKEY_KP_AND",
				"USBKEY_KP_AND2",
				"USBKEY_KP_OR",
				"USBKEY_KP_OR2",
				"USBKEY_KP_COLON",
				"USBKEY_KP_HASH",
				"USBKEY_KP_SPACE",
				"USBKEY_KP_AT",
				"USBKEY_KP_EXCLAMATION",
				"USBKEY_KP_MEM_STORE",
				"USBKEY_KP_MEM_RECALL",
				"USBKEY_KP_MEM_CLEAR",
				"USBKEY_KP_MEM_ADD",
				"USBKEY_KP_MEM_SUB",
				"USBKEY_KP_MEM_MUL",
				"USBKEY_KP_MEM_DIV",
				"USBKEY_KP_PLUS_MINUS",
				"USBKEY_KP_CLEAR",
				"USBKEY_KP_CLEAR_ENTRY",
				"USBKEY_KP_BIN",
				"USBKEY_KP_OCT",
				"USBKEY_KP_DEC",
				"USBKEY_KP_HEX",
				"reserved",
				"reserved",
				"USBKEY_LEFT_CONTROL",
				"USBKEY_LEFT_SHIFT",
				"USBKEY_LEFT_ALT",
				"USBKEY_LEFT_GUI",
				"USBKEY_RIGHT_CONTROL",
				"USBKEY_RIGHT_SHIFT",
				"USBKEY_RIGHT_ALT",
				"USBKEY_RIGHT_GUI"
			};
			try {
				static const std::chrono::milliseconds sleepDur(100);
				printf("Keyboard and mouse control taken. Waiting for test to finish.\n");
				memset(this->mapping, 0, sizeof(MapItem) * KEY_CNT);
				std::this_thread::sleep_for(sleepDur);
				for (this->curUsb = USBKEY_A; this->curUsb <= USBKEY_EXSEL; this->curUsb++) {
					this->device.keyboardPush(this->curUsb, -2);
					std::this_thread::sleep_for(sleepDur);
				}
				for (this->curUsb = USBKEY_LEFT_CONTROL; this->curUsb <= USBKEY_RIGHT_GUI; this->curUsb++) {
					this->device.keyboardPush(this->curUsb, -2);
					std::this_thread::sleep_for(sleepDur);
				}
				std::this_thread::sleep_for(sleepDur);
				int osKeyCount = 0;
				int noOsKeyCount = 0;
				int noUsbKey = 0;
				printf("OS\tUSB\tDefine");
				for (int i = 0; i < KEY_CNT; i++) {
					osKeyCount = PCF_MAX(osKeyCount, this->mapping[i].os);
					if (this->mapping[i].usb != 0 && this->mapping[i].os == 0) {
						printf("\n-\t0x%02X\t%s", unsigned(this->mapping[i].usb), usbKeyStr[this->mapping[i].usb]);
						noOsKeyCount++;
					}
				}
				qsort(this->mapping, KEY_CNT, sizeof(MapItem), [] (const void * lhs, const void * rhs) -> int {
					const MapItem * l = static_cast<const MapItem *>(lhs);
					const MapItem * r = static_cast<const MapItem *>(rhs);
					return l->os - r->os;
				});
				osKeyCount++;
				for (int key = 0, idx = 0; key < osKeyCount; key++) {
					while (key > this->mapping[idx].os) {
						if (this->mapping[idx].usb != 0 && this->mapping[idx].os != 0) {
							printf("\t0x%02X\t%s", unsigned(this->mapping[idx].usb), usbKeyStr[this->mapping[idx].usb]);
						}
						idx++;
					}
					if (key < this->mapping[idx].os) {
						printf("\n0x%04X\t0x%02X\t%s", unsigned(key), unsigned(0), usbKeyStr[0]);
						noUsbKey++;
						continue;
					}
					printf("\n0x%04X\t0x%02X\t%s", unsigned(key), unsigned(this->mapping[idx].usb), usbKeyStr[this->mapping[idx].usb]);
					idx++;
				}
				printf("\n%i OS key entries\n%i USB keys without OS key\n%i OS keys without USB key\n", osKeyCount, noOsKeyCount, noUsbKey);
			} catch (const std::exception & e) {
				fprintf(stderr, "Error: %s\n", e.what());
			} catch (...) {
				fprintf(stderr, "Error: Caught unknown exception in command worker thread.\n");
			}
			this->device.close();
		});
	}

	/**
	 * Prints the USB to OS key map.
	 *
	 * @param[in] key - USB key code
	 * @param[in] osKey - OS specific key code
	 * @param[in] action - key action
	 * @return mapped key or USBKEY_NO_EVENT to ignore this key
	 * @remark All key presses are ignores because they are looped from this instance.
	 */
	virtual uint8_t onVkvmRemapKey(const uint8_t key, const int osKey, const RemapFor action) {
		if (action != RemapFor::RF_UP) {
			if (osKey == -2) {
				this->mapping[this->curUsb].usb = this->curUsb;
			} else {
				this->mapping[this->curUsb].os = osKey;
			}
		}
		/* process only keys pressed by this application */
		return (osKey == -2) ? key : USBKEY_NO_EVENT;
	}

	/**
	 * Handles disconnects of the connected VKVM device.
	 *
	 * @param[in] reason - disconnect reason
	 */
	virtual void onVkvmDisconnected(const DisconnectReason reason) {
		switch (reason) {
		case DisconnectReason::D_USER:
			fprintf(stderr, "Info: Successfully closed VKVM device after test run.\n");
			break;
		case DisconnectReason::D_RECV_ERROR:
			fprintf(stderr, "Error: Failed to receive data from the VKVM device.\n");
			break;
		case DisconnectReason::D_SEND_ERROR:
			fprintf(stderr, "Error: Failed to send data to the VKVM device.\n");
			break;
		case DisconnectReason::D_INVALID_PROTOCOL:
			fprintf(stderr, "Error: Connected VKVM reported an unsupported protocol version.\n");
			break;
		case DisconnectReason::D_TIMEOUT:
			fprintf(stderr, "Error: Connection to the VKVM device timed out.\n");
			break;
		case DisconnectReason::COUNT:
			fprintf(stderr, "Error: VKVM device connection was closed for an unknown reason.\n");
			break;
		}
		this->device.grabGlobalInput(false);
		this->running.unlock();
	}
};


/**
 * Checks if the given string begin with 'start'.
 *
 * @param[in] start - starts with this string
 * @param[in] str - string to test against
 * @return true if 'str' starts with 'start', else false
 * @remarks This function uses strncmp() internally for string comparison.
 */
static bool startWith(const char * start, const char * str) {
	return strncmp(start, str, strlen(start)) == 0;
}


/**
 * Prints the usage description of this program.
 */
static void printHelp() {
	printf(
		"keyTest <serial>\n"
		"\n"
		"serial - path to the serial connected VKVM device\n"
	);
}


/**
 * Main entry point.
 */
#if defined(UNICODE) || defined(_UNICODE)
int asciiMain(int argc, char ** argv) {
#else /* ! UNICODE */
int main(int argc, char ** argv) {
#endif /* UNICODE */
	if (argc <= 1 || strcmp(argv[1], "-h") == 0 || startWith(argv[1], "--help")) {
		printHelp();
		return EXIT_SUCCESS;
	}
	try {
		KeyTest tester;
		if ( ! tester.start(argv[1]) ) {
			fprintf(stderr, "Error: Failed to open serial device \"%s\". Invalid port?\n", argv[1]);
			return EXIT_FAILURE;
		}
		tester.join();
	} catch (const std::exception & e) {
		fprintf(stderr, "Error: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


/**
 * Use ASCII mode even for Unicode targets.
 */
#if defined(UNICODE) || defined(_UNICODE)
extern "C" {
#ifdef __TINYC__
int _CRT_glob = 0;
#else
extern int _CRT_glob;
#endif
extern void __getmainargs(int *, char ***, char ***, int, int *);

int wmain() {
	char ** enpv, ** argv;
	int argc, si = 0;
	/* this also creates the global variable __argv */
	__getmainargs(&argc, &argv, &enpv, _CRT_glob, &si);
	return asciiMain(argc, argv);
}
} /* extern "C" */
#endif /* UNICODE */
