/**
 * @file Vkvm.hpp
 * @author Daniel Starke
 * @date 2019-10-11
 * @version 2023-10-04
 */
#ifndef __PCF_SERIAL_VKVM_HPP__
#define __PCF_SERIAL_VKVM_HPP__

#include <cstddef>
#include <cstdint>
#include <vkm-periphery/UsbKeys.hpp>


namespace pcf {
namespace serial {


/**
 * Callback interface to be implemented to receive VKVM device command responses.
 * The callback methods may be called from a different thread context.
 * Implement only these methods that shall be processed.
 */
class VkvmCallback {
public:
	enum class PeripheryResult {
		PR_OK, /**< Command was correctly received and understood by the periphery. */
		PR_BROKEN_FRAME, /**< The periphery received a broken frame. */
		PR_UNSUPPORTED_REQ_TYPE, /**< The periphery received an unsupported request type. */
		PR_INVALID_REQ_TYPE, /**< The periphery received an invalid request type. */
		PR_INVALID_FIELD_VALUE, /**< The periphery received an invalid frame field value. */
		PR_HOST_WRITE_ERROR, /**< The periphery failed to forward the request to the USB host. */
		COUNT /**< Number of possible result codes. */
	};
	enum class DisconnectReason {
		D_USER, /**< The user closed the connection. */
		D_RECV_ERROR, /**< Failed to receive data from the VKVM device. Device lost? */
		D_SEND_ERROR, /**< Failed to send data to the VKVM device. */
		D_INVALID_PROTOCOL, /**< The VKVM device protocol version does not match. */
		D_TIMEOUT, /**< An response from the VKVM device timed out. */
		COUNT /**< Number of possible disconnect reasons. */
	};
	enum class RemapFor {
		RF_DOWN, /**< key/button pressed */
		RF_UP, /**< key/button released */
		RF_PUSH /**< key/button pushed */
	};
public:
	/** Destructor. */
	virtual ~VkvmCallback() {}

	/**
	 * Called on USB periphery state change or `VkvmDevice::usbState()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] usb - new USB state
	 * @see `USBSTATE_OFF` and others
	 */
	virtual void onVkvmUsbState(const PeripheryResult res, const uint8_t usb) {}

	/**
	 * Called on keyboard LED state change or `VkvmDevice::keyboardLeds()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] leds - new LED states
	 * @see `USBLED_NUM_LOCK` and others
	 */
	virtual void onVkvmKeyboardLeds(const PeripheryResult res, const uint8_t leds) {}

	/**
	 * Called after `VkvmDevice::keyboardDown()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] key - key pressed down
	 */
	virtual void onVkvmKeyboardDown(const PeripheryResult res, const uint8_t key) {}

	/**
	 * Called after `VkvmDevice::keyboardUp()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] key - key released
	 */
	virtual void onVkvmKeyboardUp(const PeripheryResult res, const uint8_t key) {}

	/**
	 * Called after `VkvmDevice::keyboardAllUp()` completion.
	 *
	 * @param[in] res - periphery result code
	 */
	virtual void onVkvmKeyboardAllUp(const PeripheryResult res) {}

	/**
	 * Called after `VkvmDevice::keyboardPush()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] key - key pushed once
	 */
	virtual void onVkvmKeyboardPush(const PeripheryResult res, const uint8_t key) {}

	/**
	 * Called after `VkvmDevice::keyboardWrite()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] mod - key modifier used
	 * @param[in] keys - array of keys pressed once
	 * @param[in] len - number of elements in `keys`
	 */
	virtual void onVkvmKeyboardWrite(const PeripheryResult res, const uint8_t mod, const uint8_t * keys, const uint8_t len) {}

	/**
	 * Called after `VkvmDevice::mouseButtonDown()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] button - mouse button pressed down
	 */
	virtual void onVkvmMouseButtonDown(const PeripheryResult res, const uint8_t button) {}

	/**
	 * Called after `VkvmDevice::mouseButtonUp()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] button - mouse button released
	 */
	virtual void onVkvmMouseButtonUp(const PeripheryResult res, const uint8_t button) {}

	/**
	 * Called after `VkvmDevice::keyboardAllUp()` completion.
	 *
	 * @param[in] res - periphery result code
	 */
	virtual void onVkvmMouseButtonAllUp(const PeripheryResult res) {}

	/**
	 * Called after `VkvmDevice::mouseButtonPush()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] button - mouse button pushed once
	 */
	virtual void onVkvmMouseButtonPush(const PeripheryResult res, const uint8_t button) {}

	/**
	 * Called after `VkvmDevice::mouseMoveAbs()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] x - absolute mouse coordinate in x direction
	 * @param[in] y - absolute mouse coordinate in y direction
	 */
	virtual void onVkvmMouseMoveAbs(const PeripheryResult res, const int16_t x, const int16_t y) {}

	/**
	 * Called after `VkvmDevice::mouseMoveRel()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] x - relative mouse coordinate in x direction
	 * @param[in] y - relative mouse coordinate in y direction
	 */
	virtual void onVkvmMouseMoveRel(const PeripheryResult res, const int8_t x, const int8_t y) {}

	/**
	 * Called after `VkvmDevice::mouseScroll()` completion.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] wheel - relative mouse wheel offset
	 */
	virtual void onVkvmMouseScroll(const PeripheryResult res, const int8_t wheel) {}

	/**
	 * Called for each keyboard key up/down/push request to allow remapping of the keys.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] osKey - operation system specific keyboard key code
	 * @param[in] action - action performed (up/down/push)
	 * @return remapped keyboard key
	 */
	virtual uint8_t onVkvmRemapKey(const uint8_t key, const int /* osKey */, const RemapFor /* action */) { return key; }

	/**
	 * Called for each mouse button up/down/push request to allow remapping of the buttons.
	 *
	 * @param[in] res - periphery result code
	 * @param[in] osKey - operation system specific mouse button code
	 * @param[in] action - action performed (up/down/push)
	 * @return remapped mouse button
	 */
	virtual uint8_t onVkvmRemapButton(const uint8_t button, const RemapFor /* action */) { return button; }

	/**
	 * Called after successful connection to the serial connected VKVM periphery.
	 */
	virtual void onVkvmConnected() {}

	/**
	 * Called if a broken data frame has been received from the serial connected
	 * VKVM periphery.
	 */
	virtual void onVkvmBrokenFrame() {}

	/**
	 * Called if the serial connected VKVM periphery was disconnected.
	 *
	 * @param[in] reason - disconnect reason
	 */
	virtual void onVkvmDisconnected(const DisconnectReason reason) {}
};


/**
 * Class to handle a VKVM periphery device connection.
 *
 * The keyboard and mouse events are forwarded directly to the
 * VKVM periphery. The actual behavior can be found there.
 * @see vkm-periphery/Vkm.hpp
 */
class VkvmDevice {
private:
	struct Pimple; /**< Implementation defined data structure. */
	Pimple * self; /**< Implementation defined data. */
public:
public:
	explicit VkvmDevice();
	~VkvmDevice();

	VkvmDevice(const VkvmDevice &) = delete;
	VkvmDevice & operator= (const VkvmDevice &) = delete;

	bool open(VkvmCallback & cb, const char * path, const size_t timeout = 1000, const size_t tickDuration = 100);
	bool isOpen() const;
	bool isConnected() const;
	bool isFullyConnected() const;
	bool close();

	uint8_t usbState() const;
	uint8_t keyboardLeds() const;
	bool keyboardDown(const uint8_t key, const int osKey = -1);
	bool keyboardUp(const uint8_t key, const int osKey = -1);
	bool keyboardAllUp();
	bool keyboardPush(const uint8_t key, const int osKey = -1);
	bool keyboardWrite(const uint8_t mod, const uint8_t * keys, const uint8_t len);
	bool mouseButtonDown(const uint8_t button);
	bool mouseButtonUp(const uint8_t button);
	bool mouseButtonAllUp();
	bool mouseButtonPush(const uint8_t button);
	bool mouseMoveAbs(const int16_t x, const int16_t y);
	bool mouseMoveRel(const int8_t x, const int8_t y);
	bool mouseScroll(const int8_t wheel);

	bool grabGlobalInput(const bool enable);
};


} /* namespace serial */
} /* namespace pcf */


#endif /* __PCF_SERIAL_VKVM_HPP__ */
