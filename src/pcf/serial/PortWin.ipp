/**
 * @file PortWin.ipp
 * @author Daniel Starke
 * @date 2019-12-26
 * @version 2024-11-04
 */
#include <algorithm>
#include <stdexcept>
#include <libpcf/cvutf8.h>
#include <libpcf/tchar.h>
#include <pcf/serial/Port.hpp>
#include <pcf/ScopeExit.hpp>
#include <windows.h>
#include <wbemidl.h>
#include <dbt.h>


/**
 * @def PCF_PORTWIN_USE_DEVCHANGE
 * May be defined at compile time to use RegisterDeviceNotification based serial port
 * arrival/removal detection instead of polling (which is done twice per second). This, however, may
 * not work with USB-CDC devices using Microsoft's usbser.sys driver due to a long-standing bug
 * in usbser.sys. See
 * https://web.archive.org/web/20190824184346/https://e2e.ti.com/support/microcontrollers/other/f/908/t/371674
 */


#ifndef PCF_MAX_REG_PATH
#define PCF_MAX_REG_PATH 1024
#endif

#ifndef PCF_MAX_REG_KEY
#define PCF_MAX_REG_KEY 128
#endif

#ifndef PCF_MAX_REG_VALUE
#define PCF_MAX_REG_VALUE 256
#endif


namespace pcf {
namespace serial {
namespace {


/**
 * Internal class to handle serial port device change notifications.
 */
class SerialPortChangeNotifier {
private:
#ifdef PCF_PORTWIN_USE_DEVCHANGE
	HWND hWnd; /**< Handle of the Window which receives the device change notifications. */
	HDEVNOTIFY hNotify; /**< Device change notification handle. */
#else /* ! PCF_PORTWIN_USE_DEVCHANGE */
	HANDLE hTermEvent; /**< Handle to receive termination request events. */
	volatile bool terminate; /**< True if a termination requestion was received. */
#endif /* PCF_PORTWIN_USE_DEVCHANGE */
	HANDLE hThread; /**< Handle to the thread which polls for device changes. */
	std::vector<SerialPortListChangeCallback *> callbacks; /**< List of serial port device change callbacks. */
	CRITICAL_SECTION mutex; /**< Guards user API from thread operation. */
	static SerialPortChangeNotifier singleton; /**< Singleton instance of this class. */
public:
	/**
	 * Returns the single, global instance of this class.
	 *
	 * @return Class instance.
	 */
	static inline SerialPortChangeNotifier & getInstance() {
		return SerialPortChangeNotifier::singleton;
	}

	/**
	 * Adds a new serial port device change callback.
	 *
	 * @param[in] cb - callback to add
	 * @return true on success, else false
	 */
	inline bool addCallback(SerialPortListChangeCallback & cb) {
		EnterCriticalSection(&(this->mutex));
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });
		if ( ! this->isRunning() ) return false;
		for (SerialPortListChangeCallback * callback : this->callbacks) {
			if (callback == &cb) return false;
		}
		this->callbacks.push_back(&cb);
		return true;
	}

	/**
	 * Removes he given serial port device change callback.
	 *
	 * @param[in] cb - callback to remove
	 * @return true on success, else false
	 */
	inline bool removeCallback(SerialPortListChangeCallback & cb) {
		EnterCriticalSection(&(this->mutex));
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });
		if ( ! this->isRunning() ) return false;
		std::vector<SerialPortListChangeCallback *>::iterator it = this->callbacks.begin();
		for (; it != this->callbacks.end(); ++it) {
			if (*it == &cb) {
				this->callbacks.erase(it);
				return true;
			}
		}
		return false;
	}

	/**
	 * Destructor.
	 */
	inline ~SerialPortChangeNotifier() {
#ifdef PCF_PORTWIN_USE_DEVCHANGE
		if (this->hWnd != NULL) {
			PostMessage(this->hWnd, WM_CLOSE, 0, 0);
		}
#else /* ! PCF_PORTWIN_USE_DEVCHANGE */
		if (this->hTermEvent != NULL) {
			this->terminate = true;
			SetEvent(this->hTermEvent);
		}
#endif /* PCF_PORTWIN_USE_DEVCHANGE */
		if (this->hThread != NULL) {
			WaitForSingleObject(this->hThread, INFINITE);
			CloseHandle(this->hThread);
		}
#ifndef PCF_PORTWIN_USE_DEVCHANGE
		if (this->hTermEvent != NULL) {
			CloseHandle(this->hTermEvent);
		}
#endif /* ! PCF_PORTWIN_USE_DEVCHANGE */
		DeleteCriticalSection(&(this->mutex));
	}
private:
	/**
	 * Constructor.
	 */
	inline explicit SerialPortChangeNotifier():
#ifdef PCF_PORTWIN_USE_DEVCHANGE
		hWnd(NULL),
		hNotify(NULL),
#else /* ! PCF_PORTWIN_USE_DEVCHANGE */
		hTermEvent(NULL),
		terminate(false),
#endif /* PCF_PORTWIN_USE_DEVCHANGE */
		hThread(NULL)
	{
		/* create critical section */
		InitializeCriticalSection(&(this->mutex));
#ifndef PCF_PORTWIN_USE_DEVCHANGE
		/* create termination event */
		this->hTermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (this->hTermEvent == NULL) {
			throw std::runtime_error("Creation of SerialPort::SerialPortChangeNotifier termination event handle.");
		}
		ResetEvent(this->hTermEvent);
#endif /* ! PCF_PORTWIN_USE_DEVCHANGE */
		/* create thread to handle window messages */
		this->hThread = CreateThread(NULL, 0, SerialPortChangeNotifier::threadProc, this, 0, NULL);
	}

#ifdef PCF_PORTWIN_USE_DEVCHANGE
	/**
	 * Callback of the internal, hidden windows which processes device change notifications.
	 *
	 * @param[in] hWnd - handle of the window which received the message
	 * @param[in] uMsg - message received
	 * @param[in] wParam - first parameter
	 * @param[in] lParam - second parameter
	 * @return according to `DefWindowProc()`
	 */
	static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		SerialPortChangeNotifier * self = reinterpret_cast<SerialPortChangeNotifier *>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
		switch (uMsg) {
		case WM_CLOSE:
			/* the window was closed -> destroy it */
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			/* the window was destroyed -> unregister all handlers */
			if (self->hNotify != NULL) {
				UnregisterDeviceNotification(self->hNotify);
				self->hNotify = NULL;
			}
			PostQuitMessage(0);
			break;
		case WM_DEVICECHANGE:
			/* a device change notification was received -> process it */
			if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
				DEV_BROADCAST_HDR * pHdr = reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (pHdr == NULL) return 0;
				if (pHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
					DEV_BROADCAST_PORT * pP = reinterpret_cast<DEV_BROADCAST_PORT *>(pHdr);
					if (pP->dbcp_name != NULL) {
#if defined(UNICODE) || defined(_UNICODE)
						char * devPath = cvutf8_fromUtf16(pP->dbcp_name);
#else
						char * devPath = pP->dbcp_name;
#endif
						if (devPath != NULL) {
							EnterCriticalSection(&(self->mutex));
							if (wParam == DBT_DEVICEARRIVAL) {
								for (SerialPortListChangeCallback * callback : self->callbacks) {
									callback->onSerialPortArrival(devPath);
								}
							} else {
								for (SerialPortListChangeCallback * callback : self->callbacks) {
									callback->onSerialPortRemoval(devPath);
								}
							}
							LeaveCriticalSection(&(self->mutex));
#if defined(UNICODE) || defined(_UNICODE)
							free(devPath);
#endif
						}
					}
				}
			}
			break;
		default:
			/* use the default handler for all other messages */
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;
		}
		return 0;
	}
#endif /* PCF_PORTWIN_USE_DEVCHANGE */

	/**
	 * Returns true if the internal polling thread is still running.
	 *
	 * @return true if polling thread is running, else false
	 */
	inline bool isRunning() const {
		if (this->hThread == NULL) return false;
		const DWORD result = WaitForSingleObject(this->hThread, 0);
		if (result == WAIT_OBJECT_0) {
			DWORD exitCode = 0;
			if (GetExitCodeThread(this->hThread, &exitCode) == 0 || exitCode == STILL_ACTIVE) return false;
		}
		return true;
	}

	/**
	 * Internal polling thread.
	 *
	 * @param[in] lpParameter - thread parameters (this class instance)
	 */
	static DWORD WINAPI threadProc(LPVOID lpParameter) {
		if (lpParameter == NULL) return EXIT_FAILURE;
		SerialPortChangeNotifier * self = static_cast<SerialPortChangeNotifier *>(lpParameter);
#ifdef PCF_PORTWIN_USE_DEVCHANGE
		static const char * className = "SerialPort::SerialPortChangeNotifier";
		static bool wndClassRegistered = false;
		HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));
		/* window creation code needs to be within the same thread as the message loop */
		/* register window class */
		if ( ! wndClassRegistered ) {
			WNDCLASSEXA wc;
			wc.cbSize        = sizeof(WNDCLASSEXA);
			wc.style         = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc   = SerialPortChangeNotifier::windowProc;
			wc.cbClsExtra    = 0;
			wc.cbWndExtra    = 0;
			wc.hInstance     = hInstance;
			wc.hIcon         = NULL;
			wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
			wc.lpszMenuName  = NULL;
			wc.lpszClassName = className;
			wc.hIconSm       = NULL;
			if (RegisterClassExA(&wc) != 0) {
				wndClassRegistered = true;
			}
		}
		if ( ! wndClassRegistered ) {
			throw std::runtime_error("Registering window class SerialPort::SerialPortChangeNotifier failed.");
		}
		/* create window to receive device notification messages */
		self->hWnd = CreateWindowExA(
			WS_EX_CLIENTEDGE, className, className, WS_DISABLED | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1, 1,
			NULL, NULL, hInstance, NULL
		);
		if (self->hWnd == NULL) {
			throw std::runtime_error("Creation of SerialPort::SerialPortChangeNotifier window failed.");
		}
		/* bind this instance to the window */
		SetWindowLongPtrA(self->hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		/* register device notification */
		DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
		memset(&notificationFilter, 0, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
		notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		notificationFilter.dbcc_classguid  = GUID_DEVINTERFACE_COMPORT;
		self->hNotify = RegisterDeviceNotification(self->hWnd, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (self->hNotify == NULL) {
			throw std::runtime_error("Registration of SerialPort::SerialPortChangeNotifier device notifier failed.");
		}
		/* window message loop */
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#else /* ! PCF_PORTWIN_USE_DEVCHANGE */
		NativeSerialPortProvider provider;
		SerialPortList oldList, newList = provider.getSerialPortList(false);
		std::stable_sort(newList.begin(), newList.end());
		while ( ! self->terminate ) {
			switch (WaitForSingleObject(self->hTermEvent, DWORD(500) /* ms */)) {
			case WAIT_OBJECT_0:
				break;
			case WAIT_TIMEOUT:
				/* update lists */
				oldList = std::move(newList);
				newList = provider.getSerialPortList(false);
				std::stable_sort(newList.begin(), newList.end());
				/* check changes */
				{
					SerialPortList::const_iterator itOld = oldList.begin();
					const SerialPortList::const_iterator itOldEnd = oldList.end();
					SerialPortList::const_iterator itNew = newList.begin();
					const SerialPortList::const_iterator itNewEnd = newList.end();
					while (itOld != itOldEnd && itNew != itNewEnd) {
						if (*itOld < *itNew) {
							/* only in oldList */
							for (SerialPortListChangeCallback * callback : self->callbacks) {
								callback->onSerialPortRemoval(itOld->getPath());
							}
							++itOld;
						} else if (*itNew < *itOld) {
							/* only in newList */
							for (SerialPortListChangeCallback * callback : self->callbacks) {
								callback->onSerialPortArrival(itNew->getPath());
							}
							++itNew;
						} else {
							++itOld;
							++itNew;
						}
					}
					while (itOld != itOldEnd) {
						/* only in oldList */
						for (SerialPortListChangeCallback * callback : self->callbacks) {
							callback->onSerialPortRemoval(itOld->getPath());
						}
						itOld++;
					}
					while (itNew != itNewEnd) {
						/* only in newList */
						for (SerialPortListChangeCallback * callback : self->callbacks) {
							callback->onSerialPortArrival(itNew->getPath());
						}
						++itNew;
					}
				}
				break;
			default:
				return EXIT_FAILURE;
				break;
			}
		}
#endif /* PCF_PORTWIN_USE_DEVCHANGE */
		return EXIT_SUCCESS;
	}
};


/** Allocated class instance. */
SerialPortChangeNotifier SerialPortChangeNotifier::singleton;


} /* anonymous namespace */


struct NativeSerialPortProvider::Pimple {
	/**
	 * Add all available serial ports to the given list.
	 *
	 * @param[in,out] list - add available serial ports to this list
	 * @param[in,out] key - temporary buffer for regex keys
	 * @param[in,out] value - temporary buffer for regex values
	 * @return true on success, else false
	 */
	static bool getAvailablePorts(SerialPortList & list, TCHAR * key, TCHAR * value) {
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) return false;
		DWORD dwKeySize = PCF_MAX_REG_KEY;
		DWORD dwValSize = DWORD(PCF_MAX_REG_VALUE * sizeof(TCHAR));
		for (
			DWORD dwIndex = 0;
			RegEnumValue(hKey, dwIndex, key, &dwKeySize, NULL, NULL, reinterpret_cast<LPBYTE>(value), &dwValSize) == ERROR_SUCCESS;
			dwKeySize = PCF_MAX_REG_KEY, dwValSize = DWORD(PCF_MAX_REG_VALUE * sizeof(TCHAR)), ++dwIndex
		) {
			if ((dwValSize / sizeof(TCHAR)) >= PCF_MAX_REG_VALUE) continue;
			size_t j = 0;
			for (DWORD i = 0; i < dwValSize; i += DWORD(sizeof(TCHAR)), j++) {
				if (value[j] == 0) break;
				value[j] = TCHAR(_totupper(value[j]));
			}
			value[j] = 0;
#if defined(UNICODE) || defined(_UNICODE)
			SerialPort port;
			port.path = cvutf8_fromUtf16(value);
#else
			SerialPort port(value);
#endif
			if (port.getPath() != NULL) {
				list.push_back(std::move(port));
			}
		}
		RegCloseKey(hKey);
		return true;
	}

	/**
	 * Adds the friendly device names to the serial port list entries.
	 *
	 * @param[in,out] list - list of serial ports to complete
	 * @param[in,out] path - buffer with the current regex path for the friendly device names
	 * @param[in,out] key - temporary buffer for regex keys
	 * @param[in,out] value - temporary buffer for regex values
	 * @return true on success, else false
	 */
	static bool getFriendlyNames(SerialPortList & list, TCHAR * path, TCHAR * key, TCHAR * value) {
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) return false;
		const size_t oldPathLen = _tcslen(path);
		bool foundPort = false;
		DWORD dwKeySize = PCF_MAX_REG_KEY;
		/* process sub keys */
		for (
			DWORD dwIndex = 0;
			RegEnumKeyEx(hKey, dwIndex, key, &dwKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
			dwKeySize = PCF_MAX_REG_KEY, ++dwIndex
		) {
			_sntprintf(path + oldPathLen, PCF_MAX_REG_PATH - oldPathLen, _T("\\%.*") PRTCHAR, unsigned(dwKeySize), key);
			foundPort = getFriendlyNames(list, path, key, value);
			path[oldPathLen] = 0; /* revert path */
			if ( foundPort ) break;
		}
		/* process current keys */
		DWORD dwValSize = DWORD(sizeof(TCHAR) * PCF_MAX_REG_VALUE);
		if ( ! foundPort ) {
			for (
				DWORD dwIndex = 0;
				RegEnumValue(hKey, dwIndex, key, &dwKeySize, NULL, NULL, reinterpret_cast<LPBYTE>(value), &dwValSize) == ERROR_SUCCESS;
				dwKeySize = PCF_MAX_REG_KEY, dwValSize = DWORD(sizeof(TCHAR) * PCF_MAX_REG_VALUE), ++dwIndex
			) {
				/* report back if this sub key contains PortName */
				if ((dwValSize / sizeof(TCHAR)) >= PCF_MAX_REG_VALUE || dwKeySize != 8) continue;
				key[dwKeySize] = 0;
				if (_tcsicmp(key, _T("PortName")) == 0) {
					size_t j = 0;
					for (DWORD i = 0; i < dwValSize; i += DWORD(sizeof(TCHAR)), j++) {
						if (value[j] == 0) break;
						value[j] = TCHAR(_totupper(value[j]));
					}
					value[j] = 0;
					RegCloseKey(hKey);
					return dwValSize > 2 && value[0] == 'C' && value[1] == 'O' && value[2] == 'M';
				}
			}
		} else {
			/* find serial port in available serial port list */
			SerialPortList::iterator it, endIt = list.end();
#if defined(UNICODE) || defined(_UNICODE)
			char * portPath = cvutf8_fromUtf16(value);
#else
			char * portPath = value;
#endif
			for (it = list.begin(); it != endIt; ++it) {
				if (strcmp(it->getPath(), portPath) != 0) continue;
				/* complete friendly name */
				if (RegQueryValueEx(hKey, _T("FriendlyName"), NULL, NULL, reinterpret_cast<LPBYTE>(value), &dwValSize) == ERROR_SUCCESS) {
					if ((dwValSize / sizeof(TCHAR)) < PCF_MAX_REG_VALUE) {
						value[dwValSize / sizeof(TCHAR)] = 0;
#if defined(UNICODE) || defined(_UNICODE)
						it->name = cvutf8_fromUtf16(value);
#else
						it->setName(value);
#endif
					}
				}
				break;
			}
#if defined(UNICODE) || defined(_UNICODE)
			free(portPath);
#endif
		}
		RegCloseKey(hKey);
		return false;
	}
};


NativeSerialPortProvider::NativeSerialPortProvider():
	self(NULL)
{}


NativeSerialPortProvider::~NativeSerialPortProvider() {
	if (self != NULL) delete self;
}


SerialPortList NativeSerialPortProvider::getSerialPortList(const bool withNames) {
	SerialPortList result;

	/* get list of available serial ports */
	TCHAR key[PCF_MAX_REG_KEY];
	TCHAR value[PCF_MAX_REG_VALUE];
	if ( ! ( NativeSerialPortProvider::Pimple::getAvailablePorts(result, key, value) && withNames) ) return result;

	/* get friendly names for available serial ports */
	TCHAR path[PCF_MAX_REG_PATH] = _T("SYSTEM\\CurrentControlSet\\Enum");
	NativeSerialPortProvider::Pimple::getFriendlyNames(result, path, key, value);

	/* return device list */
	return result;
}


bool NativeSerialPortProvider::addNotificationCallback(SerialPortListChangeCallback & cb) {
	return SerialPortChangeNotifier::getInstance().addCallback(cb);
}


bool NativeSerialPortProvider::removeNotificationCallback(SerialPortListChangeCallback & cb) {
	return SerialPortChangeNotifier::getInstance().removeCallback(cb);
}


} /* namespace serial */
} /* namespace pcf */
