// g++ -O2 -s -static -o serListener.exe -I ../src -DHAS_STRICMP serListener.cpp ../src/pcf/serial/Port.cpp ../src/libpcf/cvutf8.c -lole32 -loleaut32 -lwbemuuid -luuid
#include <cstdio>
#include <cstdlib>
#include <pcf/serial/Port.hpp>


class Listener : public pcf::serial::SerialPortListChangeCallback {
public:
	virtual ~Listener() {}
	virtual void onSerialPortArrival(const char * port) {
		printf("inserted: '%s'\n", port);
		fflush(stdout);
	}
	virtual void onSerialPortRemoval(const char * port) {
		printf("removed: '%s'\n", port);
		fflush(stdout);
	}
};


int main() {
	Listener a;
	pcf::serial::NativeSerialPortProvider::addNotificationCallback(a);
	
	printf("Press enter to exit.\n");
	fflush(stdout);
	getchar();
	
	return EXIT_SUCCESS;
}
