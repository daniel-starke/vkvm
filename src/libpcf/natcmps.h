/**
 * @file natcmps.h
 * @author Daniel Starke
 * @see natcmps.c
 * @date 2020-01-02
 * @version 2020-01-02
 */
#ifndef __LIBPCF_NATCMPS_H__
#define __LIBPCF_NATCMPS_H__

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


int ncs_cmp(const char * lhs, const char * rhs);
int ncs_cmpi(const char * lhs, const char * rhs);


#ifdef __cplusplus
}
#endif


#endif /* __LIBPCF_NATCMPS_H__ */
