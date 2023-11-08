/**
 * @file natcmps.c
 * @author Daniel Starke
 * @see natcmps.h
 * @date 2020-01-02
 * @version 2020-01-02
 */
#include <ctype.h>
#include <libpcf/natcmps.h>


#define CHAR_T char
#define INT_T int
#define NATCMP_FUNC ncs_cmp
#define IS_DIGIT_FUNC(x) isdigit(x)
#define IS_ZERO_FUNC(x) (((x) == '0') ? 1 : 0)
#define IS_SPACE_FUNC(x) isspace(x)

/* include template function */
#include "natcmp.i"


#undef NATCMP_FUNC
#define NATCMP_FUNC ncs_cmpi
#define TO_UPPER_FUNC(x) toupper(x)

/* include template function */
#include "natcmp.i"
