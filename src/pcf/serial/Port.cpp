/**
 * @file Port.cpp
 * @author Daniel Starke
 * @date 2019-12-26
 * @version 2020-01-11
 */
#include <libpcf/target.h>


#ifdef PCF_IS_WIN
#include "PortWin.ipp"
#else
#ifdef PCF_IS_LINUX
#include "PortLinux.ipp"
#else
#error Unsupported target OS.
#endif
#endif
