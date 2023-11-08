// https://stackoverflow.com/a/1431517/2525536
// http://www.naughter.com/enumser.html
// cl winSerPortList.cpp ole32.lib oleaut32.lib wbemuuid.lib
// g++ -O2 -s -static -o winSerPortList.exe winSerPortList.cpp -lole32 -loleaut32 -lwbemuuid

#include <stdio.h>
#include <windows.h>
#include <wbemidl.h>


int main() {
	HRESULT hr = 0;
	IWbemLocator         * locator  = NULL;
	IWbemServices        * services = NULL;
	IEnumWbemClassObject * results  = NULL;
	IWbemClassObject     * result   = NULL;
	ULONG returned = 0;
	BSTR resource  = SysAllocString(L"\\\\.\\ROOT\\CIMV2");
	BSTR className = SysAllocString(L"Win32_SerialPort");
	// initialize COM
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if ( FAILED(hr) ) {
		wprintf(L"Error: Failed to initialize COM API.\n");
		return EXIT_FAILURE;
	}
	// set security level to current user
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if ( FAILED(hr) ) {
		wprintf(L"Error: Failed to get COM API permissions.\n");
		return EXIT_FAILURE;
	}
	// connect to WMI
	hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID *>(&locator));
	if ( FAILED(hr) ) {
		wprintf(L"Error: Failed to create COM instance (CLSID_WbemLocator).\n");
		return EXIT_FAILURE;
	}
	hr = locator->ConnectServer(resource, NULL, NULL, NULL, 0, NULL, NULL, &services);
	if ( FAILED(hr) ) {
		wprintf(L"Error: Failed to connect to WMI server.\n");
		return EXIT_FAILURE;
	}
	// enumerate class instances
	hr = services->CreateInstanceEnum(className, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &results);
	if ( FAILED(hr) ) {
		wprintf(L"Error: Failed to create WMI class enumeration instance. (0x%08X)\n", unsigned(hr));
		return EXIT_FAILURE;
	}
	while (results->Next(WBEM_INFINITE, 1, &result, &returned) == S_OK) {
		VARIANT var;
		VariantInit(&var);
		hr = result->Get(L"DeviceID", 0, &var, NULL, NULL);
		wprintf(L"%s", var.bstrVal);
		hr = result->Get(L"Name", 0, &var, NULL, NULL);
		wprintf(L" - %s\n", var.bstrVal);
		VariantClear(&var);
		result->Release();
	}
	// release WMI COM interfaces
	results->Release();
	services->Release();
	locator->Release();
	// unwind everything else we've allocated
	CoUninitialize();
	SysFreeString(className);
	SysFreeString(resource);
	return EXIT_SUCCESS;
}
