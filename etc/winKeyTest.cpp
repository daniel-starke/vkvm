// cl /Dsnprintf=_snprintf winKeyTest.cpp user32.lib gdi32.lib
// g++ -O2 -static -s -mwindows -o winKeyTest.exe winKeyTest.cpp -lgdi32
#include <cstdio>
#include <cstdint>
#include <windows.h>

using namespace std;

static const char * vkMap[255] = {
	"not defined",
	"VK_LBUTTON",
	"VK_RBUTTON",
	"VK_CANCEL",
	"VK_MBUTTON",
	"VK_XBUTTON1",
	"VK_XBUTTON2",
	"undefined",
	"VK_BACK",
	"VK_TAB",
	"reserved",
	"reserved",
	"VK_CLEAR",
	"VK_RETURN",
	"reserved",
	"reserved",
	"VK_SHIFT",
	"VK_CONTROL",
	"VK_MENU",
	"VK_PAUSE",
	"VK_CAPITAL",
	"VK_HANGUEL | VK_HANGUL | VK_KANA",
	"undefined",
	"VK_JUNJA",
	"VK_FINAL",
	"VK_HANJA | VK_KANJI",
	"undefined",
	"VK_ESCAPE",
	"VK_CONVERT",
	"VK_NONCONVERT",
	"VK_ACCEPT",
	"VK_MODECHANGE",
	"VK_SPACE",
	"VK_PRIOR",
	"VK_NEXT",
	"VK_END",
	"VK_HOME",
	"VK_LEFT",
	"VK_UP",
	"VK_RIGHT",
	"VK_DOWN",
	"VK_SELECT",
	"VK_PRINT",
	"VK_EXECUTE",
	"VK_SNAPSHOT",
	"VK_INSERT",
	"VK_DELETE",
	"VK_HELP",
	"'0'",
	"'1'",
	"'2'",
	"'3'",
	"'4'",
	"'5'",
	"'6'",
	"'7'",
	"'8'",
	"'9'",
	"undefined",
	"undefined",
	"undefined",
	"undefined",
	"undefined",
	"undefined",
	"undefined",
	"'a'",
	"'b'",
	"'c'",
	"'d'",
	"'e'",
	"'f'",
	"'g'",
	"'h'",
	"'i'",
	"'j'",
	"'k'",
	"'l'",
	"'m'",
	"'n'",
	"'o'",
	"'p'",
	"'q'",
	"'r'",
	"'s'",
	"'t'",
	"'u'",
	"'v'",
	"'w'",
	"'x'",
	"'y'",
	"'z'",
	"VK_LWIN",
	"VK_RWIN",
	"VK_APPS",
	"reserved",
	"VK_SLEEP",
	"VK_NUMPAD0",
	"VK_NUMPAD1",
	"VK_NUMPAD2",
	"VK_NUMPAD3",
	"VK_NUMPAD4",
	"VK_NUMPAD5",
	"VK_NUMPAD6",
	"VK_NUMPAD7",
	"VK_NUMPAD8",
	"VK_NUMPAD9",
	"VK_MULTIPLY",
	"VK_ADD",
	"VK_SEPARATOR",
	"VK_SUBTRACT",
	"VK_DECIMAL",
	"VK_DEVIDE",
	"VK_F1",
	"VK_F2",
	"VK_F3",
	"VK_F4",
	"VK_F5",
	"VK_F6",
	"VK_F7",
	"VK_F8",
	"VK_F9",
	"VK_F10",
	"VK_F11",
	"VK_F12",
	"VK_F13",
	"VK_F14",
	"VK_F15",
	"VK_F16",
	"VK_F17",
	"VK_F18",
	"VK_F19",
	"VK_F20",
	"VK_F21",
	"VK_F22",
	"VK_F23",
	"VK_F24",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"VK_NUMLOCK",
	"VK_SCROLL",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"unassigned",
	"VK_LSHIFT",
	"VK_RSHIFT",
	"VK_LCONTROL",
	"VK_RCONTROL",
	"VK_LMENU",
	"VK_RMENU",
	"VK_BROSER_BACK",
	"VK_BROSER_FORWARD",
	"VK_BROSER_REFRESH",
	"VK_BROSER_STOP",
	"VK_BROSER_SEARCH",
	"VK_BROSER_FAVORITES",
	"VK_BROSER_HOME",
	"VK_VOLUME_MUTE",
	"VK_VOLUME_DOWN",
	"VK_VOLUME_UP",
	"VK_MEDIA_NEXT_TRACK",
	"VK_MEDIA_PREV_TRACK",
	"VK_MEDIA_STOP",
	"VK_MEDIA_PLAY_PAUSE",
	"VK_LUNCH_MAIL",
	"VK_LUNCH_MEDIA_SELECT",
	"VK_LUNCH_APP1",
	"VK_LUNCH_APP2",
	"reserved",
	"reserved",
	"VK_OEM_1",
	"VK_OEM_PLUS",
	"VK_OEM_COMMA",
	"VK_OEM_MINUS",
	"VK_OEM_PERIOD",
	"VK_OEM_2",
	"VK_OEM_3",
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
	"reserved",
	"unassigned",
	"unassigned",
	"unassigned",
	"VK_OEM_4",
	"VK_OEM_5",
	"VK_OEM_6",
	"VK_OEM_7",
	"VK_OEM_8",
	"reserved",
	"OEM specific",
	"VK_OEM_102",
	"OEM specific",
	"OEM specific",
	"VK_PROCESSKEY",
	"OEM specific",
	"VK_PACKET",
	"unassigned",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"VK_ATTN",
	"VK_CRSEL",
	"VK_EXSEL",
	"VK_EREOF",
	"VK_PLAY",
	"VK_ZOOM",
	"VK_NONAME",
	"VK_PA1",
	"VK_OEM_CLEAR",
};


#define APPEND(buf, pos, ...) pos += snprintf((buf) + (pos), sizeof(buf) - (pos), __VA_ARGS__)


static HWND hWndEdit;


LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);
		FillRect(hDC, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW));
		EndPaint(hWnd, &ps);
		} break;
	case WM_SIZE: {
		RECT rect;
		GetClientRect(hWnd, &rect);
		MoveWindow(hWndEdit, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		} break;
	case WM_INPUT: {
		UINT dwSize = 0;
		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) break;
		LPBYTE lpb = static_cast<LPBYTE>(malloc(sizeof(BYTE) * dwSize));
		ZeroMemory(lpb, dwSize);
		if (lpb == NULL) break;
		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
			free(lpb);
			break;
		}
		PRAWINPUT raw = reinterpret_cast<PRAWINPUT>(lpb);
		char tBuf[4096] = {};
		int tPos = 0;
		switch (raw->header.dwType) {
		case RIM_TYPEHID: {
			APPEND(tBuf, tPos, "HID (%i bytes):", int(raw->header.dwSize));
			const DWORD size = raw->data.hid.dwSizeHid * raw->data.hid.dwCount;
			for (DWORD n = 0; n < size; n++) {
				if ((n & 0x0F) == 0) {
					APPEND(tBuf, tPos, "\r\n");
				} else {
					APPEND(tBuf, tPos, " ");
				}
				APPEND(tBuf, tPos, "0x%02X", int(raw->data.hid.bRawData[n]));
			}
			} break;
		case RIM_TYPEKEYBOARD:
			/* https://docs.microsoft.com/de-de/windows/win32/api/winuser/ns-winuser-rawkeyboard */
			APPEND(tBuf, tPos, "Keyboard (%i bytes):", int(raw->header.dwSize));
			APPEND(tBuf, tPos, "\r\nMakeCode:  0x%04X", int(raw->data.keyboard.MakeCode));
			APPEND(tBuf, tPos, "\r\nFlags:     0x%04X", int(raw->data.keyboard.Flags));
			APPEND(tBuf, tPos, "\r\nReserved:  0x%04X", int(raw->data.keyboard.Reserved));
			APPEND(tBuf, tPos, "\r\nVKey:      0x%04X", int(raw->data.keyboard.VKey));
			APPEND(tBuf, tPos, "\r\nMessage:   0x%08X", int(raw->data.keyboard.Message));
			APPEND(tBuf, tPos, "\r\nExtraInfo: 0x%08X", int(raw->data.keyboard.ExtraInformation));
			break;
		default: break;
		}
		free(lpb);
		APPEND(tBuf, tPos, "\r\n");
		for (int vk = 0; vk < 256; vk++) {
			if (GetAsyncKeyState(vk) != 0) {
				APPEND(tBuf, tPos, "\r\n%s 0x%02X %i", vkMap[vk], int(vk), int(vk));
			}
		}
		SendMessage(hWndEdit, WM_SETREDRAW, LPARAM(FALSE), 0);
		SetWindowTextA(hWndEdit, tBuf);
		SendMessage(hWndEdit, WM_SETREDRAW, LPARAM(TRUE), 0);
		UpdateWindow(hWndEdit);
		} break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, LPSTR /* pCmdLine */, int nCmdShow) {
	WNDCLASSEX wx = {};
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = windowProc;
	wx.hInstance = hInstance;
	wx.lpszClassName = TEXT("winKeyTestClass");

	if ( ! RegisterClassEx(&wx) ) {
		MessageBox(NULL, TEXT("Window class registration failed."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	HWND hWnd = CreateWindowEx(0, wx.lpszClassName, TEXT("winKeyTest"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 480, 320, NULL, NULL, wx.hInstance, NULL);
	if ( ! hWnd ) {
		MessageBox(NULL, TEXT("Window creation failed."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
	RECT rect;
	GetClientRect(hWnd, &rect);
	hWndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_DISABLED | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hWnd, NULL, NULL, NULL);
	HFONT hFont = CreateFont(0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Courier New"));
	SendMessage(hWndEdit, WM_SETFONT, WPARAM(hFont), 0);

	/* HID keyboard */
	RAWINPUTDEVICE dev = {};
	dev.usUsagePage = 0x01; /* GENERIC DESKTOP CONTROLS */
	dev.usUsage = 0x06; /* KEYBOARD */
	dev.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
	dev.hwndTarget = hWnd;

	if ( ! RegisterRawInputDevices(&dev, 1, sizeof(RAWINPUTDEVICE)) ) {
		MessageBox(NULL, TEXT("Device registration failed."), TEXT("Error"), MB_OK | MB_ICONERROR);
		CloseHandle(hFont);
		return EXIT_FAILURE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MSG msg;
	while ( GetMessage(&msg, NULL, 0, 0) ) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	CloseHandle(hFont);
	
	return EXIT_SUCCESS;
}
