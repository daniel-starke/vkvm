/**
 * @file cvutf8.h
 * @author Daniel Starke
 * @see cvutf8.c
 * @date 2014-05-03
 * @version 2018-07-14
 */
#ifndef __LIBPCF_CVUTF8_H__
#define __LIBPCF_CVUTF8_H__

#include <wchar.h>


#ifdef __cplusplus
extern "C" {
#endif


wchar_t * cvutf8_toUtf16(const char * utf8);
wchar_t * cvutf8_toUtf16N(const char * utf8, const size_t len);
char * cvutf8_fromUtf16(const wchar_t * utf16);
char * cvutf8_fromUtf16N(const wchar_t * utf16, const size_t len);


#ifdef __cplusplus
}
#endif


#endif /* __LIBPCF_CVUTF8_H__ */
