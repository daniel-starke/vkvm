/**
 * @file PortLinux.ipp
 * @author Daniel Starke
 * @date 2020-01-11
 * @version 2023-10-03
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <pcf/serial/Port.hpp>
#include <pcf/ScopeExit.hpp>
#include <pcf/UtilityLinux.hpp>
extern "C" {
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
}


#ifndef PCF_MAX_SYS_PATH
#define PCF_MAX_SYS_PATH 1024
#endif


namespace pcf {
namespace serial {
namespace {


/**
 * Internal class to handle serial port device change notifications.
 */
class SerialPortChangeNotifier {
private:
	int ed; /**< File descriptor to transmit termination request events. */
	std::thread thread; /**< Handle to the thread which polls for device changes. */
	std::vector<SerialPortListChangeCallback *> callbacks; /**< List of serial port device change callbacks. */
	std::mutex mutex; /**< Guards user API from thread operation. */
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
		std::lock_guard<std::mutex> guard(this->mutex);
		if ( ! this->thread.joinable() ) return false;
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
		std::lock_guard<std::mutex> guard(this->mutex);
		if ( ! this->thread.joinable() ) return false;
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
		if ( this->thread.joinable() ) {
			if (this->ed >= 0) {
				uint64_t term = 1;
				while (xEINTR(write, this->ed, &term, sizeof(term)) != ssize_t(sizeof(term))) usleep(10000);
			}
			this->thread.join();
			if (this->ed >= 0) close(this->ed);
		}
	}
private:
	/**
	 * Constructor.
	 */
	explicit inline SerialPortChangeNotifier():
		ed(eventfd(0, EFD_NONBLOCK))
	{
		if (this->ed < 0) throw std::runtime_error("SerialPortChangeNotifier failed to create eventfd.");
		/* create thread to handle device changes */
		this->thread = std::thread(&SerialPortChangeNotifier::threadProc, this);
	}

	/**
	 * Internal polling thread.
	 */
	void threadProc() {
		NativeSerialPortProvider provider;
		SerialPortList oldList, newList = provider.getSerialPortList(false);
		std::sort(newList.begin(), newList.end());
		fd_set readFds;
		struct timeval tout;
		for ( ;; ) {
			FD_ZERO(&readFds);
			FD_SET(this->ed, &readFds);
			/* check every 500ms */
			tout.tv_sec = 0;
			tout.tv_usec = 500000;
			errno = 0;
			const int sRes = select(this->ed + 1, &readFds, NULL, NULL, &tout);
			if (sRes < 0) {
				if (errno == EAGAIN || errno == EINTR) continue;
				break;
			}
			if (FD_ISSET(this->ed, &readFds) != 0) break;
			/* timeout -> update lists */
			try {
				oldList = std::move(newList);
				newList = provider.getSerialPortList(false);
				std::sort(newList.begin(), newList.end());
				/* check changes */
				SerialPortList::const_iterator itOld = oldList.begin();
				const SerialPortList::const_iterator itOldEnd = oldList.end();
				SerialPortList::const_iterator itNew = newList.begin();
				const SerialPortList::const_iterator itNewEnd = newList.end();
				while (itOld != itOldEnd && itNew != itNewEnd) {
					if (*itOld < *itNew) {
						/* only in oldList */
						for (SerialPortListChangeCallback * callback : this->callbacks) {
							callback->onSerialPortRemoval(itOld->getPath());
						}
						++itOld;
					} else if (*itNew < *itOld) {
						/* only in newList */
						for (SerialPortListChangeCallback * callback : this->callbacks) {
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
					for (SerialPortListChangeCallback * callback : this->callbacks) {
						callback->onSerialPortRemoval(itOld->getPath());
					}
					itOld++;
				}
				while (itNew != itNewEnd) {
					/* only in newList */
					for (SerialPortListChangeCallback * callback : this->callbacks) {
						callback->onSerialPortArrival(itNew->getPath());
					}
					++itNew;
				}
			} catch (...) {}
		}
	}
};


/** Allocated class instance. */
SerialPortChangeNotifier SerialPortChangeNotifier::singleton;


} /* anonymous namespace */


struct NativeSerialPortProvider::Pimple {
	/**
	 * Returns the parent file path which includes the `idProduct` file.
	 *
	 * @param[in] path - start with this path
	 * @param[in] baseLen - do not traverse upwards beyond this point
	 * @param[in] pathLen - current path length in characters
	 * @return path with `idProduct` or NULL on error
	 */
	static inline char * getParentWithIdProduct(char * path, const size_t baseLen, size_t pathLen) {
		if (path == NULL || baseLen <= 0) return NULL;
		struct stat st;
		while (baseLen < pathLen) {
			char * parentEnd = strrchr(path, '/');
			if (parentEnd == NULL) break;
			pathLen = size_t(pathLen - strlen(parentEnd));
			const ssize_t subPathLen = snprintf(path + pathLen + 1, PCF_MAX_SYS_PATH - pathLen - 1, "idProduct");
			if (subPathLen > 0 && subPathLen < ssize_t(PCF_MAX_SYS_PATH - pathLen - 1)) {
				if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
					parentEnd[1] = 0;
					return path;
				}
			}
			*parentEnd = 0;
		}
		path[baseLen] = 0;
		return NULL;
	}

	/**
	 * Add all available serial ports to the given list.
	 *
	 * @param[in,out] list - add available serial ports to this list
	 * @param[in] path - buffer with the current search path for serial devices device names
	 * @param[in] withNames - set true to search for serial port device names (takes more time)
	 * @return true on success, else false
	 */
	static bool getAvailablePorts(SerialPortList & list, char * path, const bool withNames) {
		struct stat st;
		struct dirent ** dirList;
		const int count = xEINTR(scandir, path, &dirList, nullptr, nullptr);
		if (count < 0) return false;
		const auto freeDirListOnReturn = makeScopeExit([=]() {
			for (int n = 0; n < count; n++) free(dirList[n]);
			free(dirList);
		});
		char * buffers = static_cast<char *>(malloc(3 * PCF_MAX_SYS_PATH * sizeof(char)));
		if (buffers == NULL) return false;
		const auto freeBuffersOnReturn = makeScopeExit([=]() { free(buffers); });
		char * buffer0 = buffers;
		char * buffer1 = buffers + PCF_MAX_SYS_PATH;
		char * buffer2 = buffers + (2 * PCF_MAX_SYS_PATH);
		const size_t origPathLen = strlen(path);
		const size_t remPathLen = size_t((origPathLen < PCF_MAX_SYS_PATH) ? PCF_MAX_SYS_PATH - origPathLen : 0);
		SerialPort newPort;
		for (int n = 0; n < count; n++) {
			if (strcmp(dirList[n]->d_name, ".") == 0) continue;
			if (strcmp(dirList[n]->d_name, "..") == 0) continue;
			int pathSubLen = snprintf(path + origPathLen, remPathLen, "/%s/device/driver", dirList[n]->d_name);
			if (pathSubLen < 1 || pathSubLen >= int(remPathLen)) continue;
			if (xEINTR(lstat, path, &st) != 0 || ( ! S_ISLNK(st.st_mode) )) continue; /* no device driver assigned */
			const ssize_t linkLen = xEINTR(readlink, path, buffer0, PCF_MAX_SYS_PATH);
			if (linkLen <= 0 || linkLen >= PCF_MAX_SYS_PATH) continue; /* invalid or too long symbolic link */
			buffer0[linkLen] = 0;
			/* build final device path */
			const ssize_t devPathLen = snprintf(buffer1, PCF_MAX_SYS_PATH, "/dev/%s", dirList[n]->d_name);
			if (devPathLen <= 0 || devPathLen >= PCF_MAX_SYS_PATH) continue;
			buffer1[devPathLen] = 0;
			newPort = std::move(SerialPort(buffer1));
			/* build friendly name from driver name */
			char * friendlyName = strrchr(buffer0, '/');
			if (friendlyName == NULL) continue;
			friendlyName++;
			if (strcmp(friendlyName, "serial8250") == 0) {
				/* special handling to check if the serial port is really available or just a dummy */
				const int fd = xEINTR(open, newPort.getPath(), O_RDWR | O_NONBLOCK | O_NOCTTY);
				if (fd < 0 && errno != EAGAIN && errno != EBUSY) continue; /* device cannot be opened */
				if (fd >= 0 && errno != EAGAIN && errno != EBUSY) {
					struct serial_struct info;
					if (xEINTR(ioctl, fd, TIOCGSERIAL, &info) == 0 && info.type == PORT_UNKNOWN) {
						close(fd);
						continue; /* PORT_UNKNOWN devices are ignored */
					}
				}
				close(fd);
			}
			/* get name for USB devices: readlink ./device, get parent directory, read product file */
			while ( withNames ) {
				/* resolve path of /sys/class/tty/xxx */
				path[origPathLen + pathSubLen - 14] = 0;
				memcpy(buffer1, path, origPathLen);
				buffer1[origPathLen] = '/';
				buffer1[origPathLen + 1] = 0;
				/* obtain full path by appending symbolic link to /sys/class/tty/ */
				const ssize_t devLinkPathLen = xEINTR(readlink, path, buffer1 + origPathLen + 1, size_t(PCF_MAX_SYS_PATH - origPathLen - 1));
				if (devLinkPathLen <= 0 || devLinkPathLen >= ssize_t(PCF_MAX_SYS_PATH - origPathLen - 1)) break;
				/* assume this serial port is owned by an USB device and get its product name */
				size_t basePathLen = size_t(origPathLen + devLinkPathLen + 1);
				buffer1[basePathLen] = 0;
				const size_t extOrigPathLen = origPathLen + ((strncmp("../../devices/", buffer1 + origPathLen + 1, 14) == 0) ? 15 : 1);
				char * idProductPath = getParentWithIdProduct(buffer1, extOrigPathLen, basePathLen);
				if (idProductPath == NULL) break; /* device has no parent with a idProduct file -> is not a USB device */
				basePathLen = strlen(buffer1);
				const ssize_t productPathLen = snprintf(buffer1 + basePathLen, PCF_MAX_SYS_PATH - basePathLen, "product");
				if (productPathLen < 0 || productPathLen >= ssize_t(PCF_MAX_SYS_PATH - basePathLen)) break; /* path too long */
				buffer1[basePathLen + productPathLen] = 0;
				const int fd = xEINTR(open, buffer1, O_RDONLY);
				if (fd < 0) break; /* failed to open USB product file */
				const ssize_t bytesRead = xEINTR(read, fd, buffer2, PCF_MAX_SYS_PATH - 1);
				close(fd);
				if (bytesRead <= 0) break; /* empty USB product name */
				buffer2[bytesRead] = 0;
				for (ssize_t k = 0; k < bytesRead; k++) {
					if (buffer2[k] < ' ') {
						/* do not include control characters */
						buffer2[k] = 0;
						break;
					}
				}
				/* successfully derived product name of the owning USB device */
				friendlyName = buffer2;
				break;
			};
			/* add port to list if we got to this point */
			newPort.setName(friendlyName);
			list.push_back(std::move(newPort));
		}
		return true;
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
	char path[PCF_MAX_SYS_PATH] = "/sys/class/tty";
	NativeSerialPortProvider::Pimple::getAvailablePorts(result, path, withNames);

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
