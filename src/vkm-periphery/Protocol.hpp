/**
 * @file Protocol.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-10-11
 * @version 2023-10-24
 */
#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__


#define VKVM_PROT_VERSION 0x0100
#define VKVM_PROT_SPEED 115200
#define VKVM_MAX_FRAME_SIZE 256


/** Namespace for the response type enumeration. */
struct ResponseType {
	/**
	 * Possible result frame types.
	 *
	 *  <0x40        - successful response
	 * >=0x40 < 0x60 - interrupt
	 * >=0x60 < 0x80 - debug
	 * >=0x80        - error
	 */
	enum Type {
		S_OK                   = 0x00, /**< request type specific response */
		I_USB_STATE_UPDATE     = 0x40, /**< `uint8_t` with the updated periphery USB state */
		I_LED_UPDATE           = 0x41, /**< `uint8_t` with the updated LED states */
		D_MESSAGE              = 0x60, /**< `uint8_t` array with the debug message */
		E_BROKEN_FRAME         = 0x80, /**< empty frame */
		E_UNSUPPORTED_REQ_TYPE = 0x81, /**< `uint8_t` with the received request type */
		E_INVALID_REQ_TYPE     = 0x82, /**< `uint8_t` with the received request type */
		E_INVALID_FIELD_VALUE  = 0x83, /**< `uint8_t` with the index of the invalid field */
		E_HOST_WRITE_ERROR     = 0x85, /**< failed to send data to the host */
	};
};


/** Namespace for the request type enumeration. */
struct RequestType {
	/**
	 * Possible request types.
	 *
	 * @remarks Only expand this in the given order to ensure downward compatibility.
	 */
	enum Type {
		GET_PROTOCOL_VERSION,     /**< return the protocol version; shall match `VKVM_PROT_VERSION` */
		GET_ALIVE,                /**< return `ResponseType::S_OK` */
		GET_USB_STATE,            /**< return `uint8_t` with the periphery USB connection state */
		GET_KEYBOARD_LEDS,        /**< return `uint8_t` with keyboard LED states */
		SET_KEYBOARD_DOWN,        /**< return `uint8_t` with a bitmap of the keys actually pressed (LSB is the first key) */
		SET_KEYBOARD_UP,          /**< return `uint8_t` with a bitmap of the keys actually released (LSB is the first key) */
		SET_KEYBOARD_ALL_UP,      /**< return `ResponseType::S_OK` */
		SET_KEYBOARD_PUSH,        /**< return `uint8_t` with the number of characters pushed */
		SET_KEYBOARD_WRITE,       /**< return `uint8_t` with the number of characters written */
		SET_MOUSE_BUTTON_DOWN,    /**< return `ResponseType::S_OK` */
		SET_MOUSE_BUTTON_UP,      /**< return `ResponseType::S_OK` */
		SET_MOUSE_BUTTON_ALL_UP,  /**< return `ResponseType::S_OK` */
		SET_MOUSE_BUTTON_PUSH,    /**< return `uint8_t` with the number of buttons pushed */
		SET_MOUSE_MOVE_ABS,       /**< return `ResponseType::S_OK` */
		SET_MOUSE_MOVE_REL,       /**< return `ResponseType::S_OK` */
		SET_MOUSE_SCROLL,         /**< return `ResponseType::S_OK` */
		COUNT                     /**< number of possible request types */
	};
};


#endif /* __PROTOCOL_HPP__ */
