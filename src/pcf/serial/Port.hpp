/**
 * @file Port.hpp
 * @author Daniel Starke
 * @date 2019-12-26
 * @version 2023-10-03
 */
#ifndef __PCF_SERIAL_PORT_HPP__
#define __PCF_SERIAL_PORT_HPP__

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <libpcf/natcmps.h>


namespace pcf {
namespace serial {


/* forward declaration */
class NativeSerialPortProvider;


/**
 * Description of a single serial port.
 */
class SerialPort {
private:
	char * path;
	char * name;
	friend NativeSerialPortProvider;
public:
	/**
	 * Constructor.
	 *
	 * @param[in] p - unique path of the serial port
	 * @param[in] n - human readable serial port name
	 */
	explicit inline SerialPort(const char * p = NULL, const char * n = NULL):
		path(NULL),
		name(NULL)
	{
		this->initFrom(p, n);
	}

	/**
	 * Copy constructor.
	 *
	 * @param[in] o - object to copy
	 */
	inline SerialPort(const SerialPort & o):
		path(NULL),
		name(NULL)
	{
		this->initFrom(o.path, o.name);
	}

	/**
	 * Move constructor.
	 *
	 * @param[in,out] o - object to move
	 */
	inline SerialPort(SerialPort && o):
		path(o.path),
		name(o.name)
	{
		o.path = NULL;
		o.name = NULL;
	}

	/** Destructor. */
	inline ~SerialPort() {
		if (this->path != NULL) free(this->path);
		if (this->name != NULL) free(this->name);
	}

	/**
	 * Assignment operator.
	 *
	 * @param[in] o - object to assign
	 */
	inline SerialPort & operator= (const SerialPort & o) {
		if (this != &o) {
			if (this->path != NULL) {
				free(this->path);
				this->path = NULL;
			}
			if (this->name != NULL) {
				free(this->name);
				this->name = NULL;
			}
			this->initFrom(o.path, o.name);
		}
		return *this;
	}

	/**
	 * Move-assignment operator.
	 *
	 * @param[in,out] o - object to move
	 */
	inline SerialPort & operator= (SerialPort && o) {
		if (this != &o) {
			if (this->path != NULL) free(this->path);
			if (this->name != NULL) free(this->name);
			this->path = o.path;
			this->name = o.name;
			o.path = NULL;
			o.name = NULL;
		}
		return *this;
	}

	/**
	 * Less-than comparison operator.
	 *
	 * @param[in] o - object to compare with
	 * @remarks Comparison is solely done by the path attribute.
	 */
	inline bool operator< (const SerialPort & o) const {
		return this->comparePaths(o) < 0;
	}

	/**
	 * Equality comparison operator.
	 *
	 * @param[in] o - object to compare with
	 * @remarks Comparison is solely done by the path attribute.
	 */
	inline bool operator== (const SerialPort & o) const {
		return this->comparePaths(o) == 0;
	}

	/**
	 * Inequality comparison operator.
	 *
	 * @param[in] o - object to compare with
	 * @remarks Comparison is solely done by the path attribute.
	 */
	inline bool operator!= (const SerialPort & o) const {
		return this->comparePaths(o) != 0;
	}

	/**
	 * Swaps the content of this and the passed object.
	 *
	 * @param[in,out] o - object to swap with
	 */
	inline void swap(SerialPort & o) {
		using std::swap;
		swap(this->path, o.path);
		swap(this->name, o.name);
	}

	/**
	 * Returns the unique path of the serial port. The returned pointer shell not be freed.
	 *
	 * @return serial port path
	 * @remarks The function is not re-entrant safe.
	 */
	inline const char * getPath() const {
		return this->path;
	}

	/**
	 * Returns the human readable name of the serial port. The returned pointer shell not be freed.
	 *
	 * @return serial port name
	 * @remarks The function is not re-entrant safe.
	 */
	inline const char * getName() const {
		return this->name;
	}

	/**
	 * Sets the human readable name of the serial port.
	 *
	 * @param[in] n - human readable serial port name
	 */
	inline void setName(const char * n) {
		if (this->name != NULL) free(this->name);
		if (n == NULL) {
			this->name = NULL;
			return;
		}
		const size_t len = strlen(n) + 1;
		this->name = static_cast<char *>(malloc(len * sizeof(char)));
		if (this->name != NULL) memcpy(this->name, n, len * sizeof(char));
	}
private:
	/**
	 * Initializes the object from the given parameters.
	 *
	 * @param[in] p - unique path of the serial port
	 * @param[in] n - human readable serial port name
	 */
	inline void initFrom(const char * p, const char * n) {
		if (p != NULL) {
			const size_t len = strlen(p) + 1;
			this->path = static_cast<char *>(malloc(len * sizeof(char)));
			if (this->path != NULL) memcpy(this->path, p, len * sizeof(char));
			this->setName(n);
		}
	}

	/**
	 * Returns the result of strcmp() for both paths with correct handling of null pointers.
	 *
	 * @param[in] o - object to compare with
	 * @return <0 if this < o; ==0 if this == o; >0 if this > o
	 */
	inline int comparePaths(const SerialPort & o) const {
		if (this->path == NULL && o.path == NULL) return 0;
		if (this->path == NULL) return -1;
		if (o.path == NULL) return 1;
		return strcmp(this->path, o.path);
	}
};


/**
 * Serial port list. See SerialPort on how to receive this list.
 */
typedef std::vector<SerialPort> SerialPortList;


/**
 * Callback interface to be implemented to receive serial port change notifications.
 */
class SerialPortListChangeCallback {
public:
	/** Destructor. */
	virtual ~SerialPortListChangeCallback() {}

	/**
	 * Called if a new serial port was detected.
	 *
	 * @param[in] port - added serial port path
	 * @remarks This may be called from a different thread.
	 */
	virtual void onSerialPortArrival(const char * port) = 0;

	/**
	 * Called if a serial port was removed.
	 *
	 * @param[in] port - removed serial port path
	 * @remarks This may be called from a different thread.
	 */
	virtual void onSerialPortRemoval(const char * port) = 0;
};


/**
 * OS native serial port list provider. Use this to receive a list of serial ports available.
 */
class NativeSerialPortProvider {
private:
	struct Pimple; /**< Implementation defined data structure. */
	Pimple * self; /**< Implementation defined data. */
public:
	/** Constructor. */
	explicit NativeSerialPortProvider();
	/** Destructor. */
	~NativeSerialPortProvider();

	/**
	 * Returns a list of available serial ports.
	 *
	 * @param[in] withNames - set to get list with human readable serial ports names
	 * @return serial port list
	 */
	SerialPortList getSerialPortList(const bool withNames = true);

	/**
	 * Add a callback which is called on serial port insertion or removal.
	 *
	 * @param[in] cb - callback instance
	 * @return true on success, else false
	 * @remarks The callback may be called from a different thread.
	 */
	static bool addNotificationCallback(SerialPortListChangeCallback & cb);

	/**
	 * Remove the given notification callback.
	 *
	 * @param[in] cb - callback instance
	 * @return true on success, else false
	 */
	static bool removeNotificationCallback(SerialPortListChangeCallback & cb);
};


} /* namespace serial */
} /* namespace pcf */


namespace std {


/**
 * Swaps the content of both passed objects.
 *
 * @param[in,out] lhs - left-hand statement
 * @param[in,out] rhs - right-hand statement
 */
inline static void swap(pcf::serial::SerialPort & lhs, pcf::serial::SerialPort & rhs) {
	lhs.swap(rhs);
}


} /* namespace std */


#endif /* __PCF_SERIAL_PORT_HPP__ */
