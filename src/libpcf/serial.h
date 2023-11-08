/**
 * @file serial.h
 * @author Daniel Starke
 * @date 2019-03-26
 * @version 2023-06-06
 */
#ifndef __LIBPCF_SERIAL_H__
#define __LIBPCF_SERIAL_H__

#include <stdint.h>
#include "target.h"
#ifdef PCF_IS_LINUX
#include <sys/types.h>
#endif /* PCF_IS_LINUX */


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Defines the possible serial data framing types.
 */
typedef enum {
	SFR_7N1, /**< 7 data bits, no parity, 1 stop bit */
	SFR_8N1, /**< 8 data bits, no parity, 1 stop bit */
	SFR_7N2, /**< 7 data bits, no parity, 2 stop bits */
	SFR_8N2, /**< 8 data bits, no parity, 2 stop bits */
	SFR_7E1, /**< 7 data bits, even parity, 1 stop bit */
	SFR_8E1, /**< 8 data bits, even parity, 1 stop bit */
	SFR_7E2, /**< 7 data bits, even parity, 2 stop bits */
	SFR_8E2, /**< 8 data bits, even parity, 2 stop bits */
	SFR_7O1, /**< 7 data bits, odd parity, 1 stop bit */
	SFR_8O1, /**< 8 data bits, odd parity, 1 stop bit */
	SFR_7O2, /**< 7 data bits, odd parity, 2 stop bits */
	SFR_8O2  /**< 8 data bits, odd parity, 2 stop bits */
} tSerFraming;


/**
 * Defines the possible serial flow control types.
 */
typedef enum {
	SFC_NONE, /**< disable flow control */
	SFC_SW, /**< use software based flow control (XON/XOFF) */
	SFC_HW /**< use hardware based flow control (RTS/CTS) */
} tSerFlowCtrl;


/**
 * Defines possible serial status line bits.
 */
typedef enum {
	SL_RTS = 0x01,
	SL_CTS = 0x02,
	SL_DSR = 0x04,
	SL_DCD = 0x08,
	SL_DTR = 0x10,
	SL_RING = 0x20
} tSerStatusLine;


/**
 * Defines possible serial operation errors.
 */
typedef enum {
	SE_SUCCESS,
	SE_OUT_OF_MEMORY,
	SE_INVALID_ARG,
	SE_PATH_NOT_FOUND,
	SE_NO_PERMISSIONS,
	SE_TIMEOUT,
	SE_BUSY,
	SE_UNKNOWN
} tSerError;


/**
 * @internal target specific
 */
typedef struct tSerial tSerial;


tSerial * ser_create(const char * device, const size_t speed, const tSerFraming framing, const tSerFlowCtrl flow);
int ser_setConfig(tSerial * ser, const size_t speed, const tSerFraming framing, const tSerFlowCtrl flow);
tSerStatusLine ser_getLines(tSerial * ser);
int ser_setLines(tSerial * ser, const tSerStatusLine status);
ssize_t ser_read(tSerial * ser, uint8_t * buf, const size_t size, const size_t timeout);
ssize_t ser_write(tSerial * ser, const uint8_t * buf, const size_t size, const size_t timeout);
int ser_clear(tSerial * ser);
void ser_delete(tSerial * ser);
tSerError ser_lastError(void);


#ifdef __cplusplus
}
#endif


#endif /* __LIBPCF_SERIAL_H__ */
