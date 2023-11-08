// http://www.naughter.com/enumser.html
// cl winSerPortList2.cpp
// g++ -O2 -s -static -municode -o winSerPortList2.exe winSerPortList2.cpp

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <windows.h>

namespace std {
#ifdef UNICODE
typedef wstring tstring;
#define TCHAR wchar_t
#define _T(x) L##x
#define _tmain wmain
#define totupper towupper
#define _tprintf wprintf
#define tcsicmp _wcsicmp
#else
typedef string tstring;
#define TCHAR char
#define _T(x) x
#define _tmain main
#define totupper toupper
#define _tprintf printf
#define tcsicmp _stricmp
#endif
} /* namespace std */


typedef std::map<std::tstring, std::tstring> PortMap;


void getSerialPorts(PortMap & map) {
	const auto getPorts = [] (PortMap & map) {
		HKEY hKey;
		LSTATUS res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_QUERY_VALUE, &hKey);
		if (res != ERROR_SUCCESS) return;
		TCHAR name[128];
		DWORD dwNameSize = 128;
		BYTE lpData[128 * sizeof(TCHAR)];
		DWORD dwSize = 128 * sizeof(TCHAR);
		DWORD dwIndex = 0;
		while (RegEnumValue(hKey, dwIndex, name, &dwNameSize, NULL, NULL, lpData, &dwSize) == ERROR_SUCCESS) {
			std::tstring value(reinterpret_cast<const TCHAR *>(lpData), size_t(dwSize) / sizeof(TCHAR));
			value.erase(std::find(value.begin(), value.end(), '\0'), value.end());
			for (TCHAR & c : value) c = TCHAR(totupper(c));
			map[value] = std::tstring();
			dwNameSize = 128;
			dwSize = 128 * sizeof(TCHAR);
			++dwIndex;
		}
		RegCloseKey(hKey);
	};
	const std::function<std::tstring(PortMap &, const std::tstring &)> getFriendlyNames = [&getFriendlyNames] (PortMap & map, const std::tstring & path) -> std::tstring {
		LSTATUS res;
		HKEY hKey;
		res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);
		if (res != ERROR_SUCCESS) return std::tstring();
		TCHAR name[128];
		DWORD dwNameSize = 128;
		DWORD dwIndex = 0;
		std::tstring port;
		while (RegEnumKeyEx(hKey, dwIndex, name, &dwNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			port = getFriendlyNames(map, path + _T("\\") + std::tstring(reinterpret_cast<const TCHAR *>(name), size_t(dwNameSize)));
			if ( ! port.empty() ) break;
			dwNameSize = 128;
			++dwIndex;
		}
		BYTE lpData[256 * sizeof(TCHAR)];
		DWORD dwSize = 256 * sizeof(TCHAR);
		if ( port.empty() ) {
			dwIndex = 0;
			while (RegEnumValue(hKey, dwIndex, name, &dwNameSize, NULL, NULL, lpData, &dwSize) == ERROR_SUCCESS) {
				/* node contains PortName -> report back */
				if (tcsicmp(name, _T("PortName")) == 0) {
					std::tstring value(reinterpret_cast<const TCHAR *>(lpData), size_t(dwSize) / sizeof(TCHAR));
					value.erase(std::find(value.begin(), value.end(), '\0'), value.end());
					for (TCHAR & c : value) c = TCHAR(totupper(c));
					RegCloseKey(hKey);
					return value;
				}
				dwNameSize = 128;
				dwSize = 256 * sizeof(TCHAR);
				++dwIndex;
			}
		} else if (map.find(port) != map.end()) {
			if (RegQueryValueEx(hKey, _T("FriendlyName"), NULL, NULL, lpData, &dwSize) == ERROR_SUCCESS) {
				map[port] = std::tstring(reinterpret_cast<const TCHAR *>(lpData), size_t(dwSize) / sizeof(TCHAR));
			}
		}
		RegCloseKey(hKey);
		return std::tstring();
	};
	getPorts(map);
	getFriendlyNames(map, std::tstring(_T("SYSTEM\\CurrentControlSet\\Enum")));
}


int _tmain() {
	PortMap ports;
	getSerialPorts(ports);
	
	for (const auto & port : ports) {
#ifdef UNICODE
		_tprintf(_T("%S - %S\n"), port.first.c_str(), port.second.c_str());
#else
		_tprintf(_T("%s - %s\n"), port.first.c_str(), port.second.c_str());
#endif
	}
	
	return EXIT_SUCCESS;
}
