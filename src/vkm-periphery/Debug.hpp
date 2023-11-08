/**
 * @file Debug.hpp
 * @author Daniel Starke
 * @copyright Copyright 2022-2023 Daniel Starke
 * @date 2022-08-13
 * @version 2023-10-24
 */
#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__


#ifdef DEBUG
#include "Framing.hpp"
#include "Protocol.hpp"


extern Framing<VKVM_MAX_FRAME_SIZE> framing;


/**
 * Writes the given string as debug message to the controller.
 *
 * @param[in] x - string to record
 */
#define DBG_MSG(x) \
	do { \
		framing.beginTransmission(0); \
		framing.write(uint8_t(ResponseType::D_MESSAGE)); \
		framing.write(reinterpret_cast<const uint8_t *>((x)), strlen((x))); \
		framing.endTransmission(); \
	} while (false);
#else /* DEBUG */
#define DBG_MSG(x)
#endif /* not DEBUG */


#endif /* __DEBUG_HPP__ */
