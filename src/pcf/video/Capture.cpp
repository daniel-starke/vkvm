/**
 * @file Capture.cpp
 * @author Daniel Starke
 * @date 2019-10-03
 * @version 2020-01-12
 */
#include <libpcf/target.h>


#ifdef PCF_IS_WIN
#include "CaptureDirectShow.ipp"
#else
#ifdef PCF_IS_LINUX
#include "CaptureVideo4Linux2.ipp"
#else
#error Unsupported target OS.
#endif
#endif
