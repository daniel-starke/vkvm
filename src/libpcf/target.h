/**
 * @file target.h
 * @author Daniel Starke
 * @date 2017-12-02
 * @version 2023-05-20
 */
#ifndef __LIBPCF_TARGET_H__
#define __LIBPCF_TARGET_H__


#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>


#ifndef PCF_PI
/** Defines double PI constant. */
#define PCF_PI 3.14159265358979323846
#endif


#ifndef PCF_DEG_TO_RAD
/** Converts the given value from degrees to radians in the specified type. */
#define PCF_DEG_TO_RAD(type, x) (type(x) * type(PCF_PI) / type(180))
#endif


#ifndef PCF_RAD_TO_DEG
/** Converts the given value from radians to degrees in the specified type. */
#define PCF_RAD_TO_DEG(type, x) (type(x) * type(180) / type(PCF_PI))
#endif


#ifndef PCF_TO_STR
/** Converts the passed argument to a string literal. */
# define PCF_TO_STR(x) #x
#endif /* to string */


#if defined(__WIN32__) || defined(__WIN64__) || defined(WIN32) \
 || defined(WINNT) || defined(_WIN32) || defined(__WIN32) || defined(__WINNT) \
 || defined(__MINGW32__) || defined(__MINGW64__)
# ifndef PCF_IS_WIN
/** Defined if compiler target is windows. */
#  define PCF_IS_WIN 1
# endif
# undef PCF_IS_NO_WIN
#else /* no windows */
# ifndef PCF_IS_NO_WIN
/** Defined if compiler target is _not_ windows. */
#  define PCF_IS_NO_WIN 1
# endif
# undef PCF_IS_WIN
#endif /* windows */


#if !defined(PCF_IS_WIN) && (defined(unix) || defined(__unix) || defined(__unix__) \
 || defined(__gnu_linux__) || defined(linux) || defined(__linux) \
 || defined(__linux__))
# ifndef PCF_IS_LINUX
/** Defined if compiler target is linux/unix. */
# define PCF_IS_LINUX 1
# endif
# undef PCF_IS_NO_LINUX
#else /* no linux */
# ifndef PCF_IS_NO_LINUX
/** Defined if compiler target is _not_ linux/unix. */
#  define PCF_IS_NO_LINUX 1
# endif
# undef PCF_IS_LINUX
#endif /* linux */


#if defined(_X86_) || defined(i386) || defined(__i386__)
# ifndef PCF_IS_X86
/** Defined if compiler target is x86. */
# define PCF_IS_X86
# endif
#else /* no x86 */
# ifndef PCF_IS_NO_X86
/** Defined if compiler target _not_ is x86. */
# define PCF_IS_NO_X86
# endif
#endif /* x86 */


#if defined(__amd64__) || defined(__x86_64__)
# ifndef PCF_IS_X64
/** Defined if compiler target is x64. */
# define PCF_IS_X64
# endif
#else /* no x64 */
# ifndef PCF_IS_NO_X64
/** Defined if compiler target _not_ is x64. */
# define PCF_IS_NO_X64
# endif
#endif /* x64 */


#ifdef PCF_PATH_SEP
# undef PCF_PATH_SEP
#endif
#ifdef PCF_IS_WIN
/** Defines the Windows path separator. */
# define PCF_PATH_SEP "\\"
# define PCF_PATH_SEPU L"\\"
#else /* PCF_IS_NO_WIN */
/** Defines the non-Windows path separator. */
# define PCF_PATH_SEP "/"
# define PCF_PATH_SEPU L"/"
#endif /* PCF_IS_WIN */


/* restrict pointer excess, see C99 restrict */
#ifndef PCF_RESTRICT
#if defined(__clang__)
# define PCF_RESTRICT __restrict
#elif defined(__ICC) && __ICC > 1110
# define PCF_RESTRICT __restrict
#elif defined(__GNUC__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
# define PCF_RESTRICT __restrict
#elif defined(_MSC_VER) && _MSC_VER >= 1400
# define PCF_RESTRICT __restrict
#else
# define PCF_RESTRICT
#endif
#endif /* PCF_RESTRICT */


/* align variable like: float PCF_ALIGNAS(8) myVar64Aligned; */
#ifndef PCF_ALIGNAS
#if defined(__clang__)
# define PCF_ALIGNAS(bytes) __attribute__((aligned(bytes * 8)))
#elif defined(__ICC) && __ICC > 1110
# define PCF_ALIGNAS(bytes) __declspec(align(bytes * 8))
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
# define PCF_ALIGNAS(bytes) __attribute__((aligned(bytes * 8)))
#elif defined(_MSC_VER) && _MSC_VER >= 1400
# define PCF_ALIGNAS(bytes) __declspec(align(bytes * 8))
#else
# define PCF_ALIGNAS(bytes)
#endif
#endif /* PCF_ALIGNAS */


/* assume passed pointer is aligned to given size */
#ifndef PCF_ASSUME_ALIGNED_AS
#if defined(__clang__) && ((__clang_major__ > 3) || (__clang_major__ == 3 && __clang_minor__ >= 6))
# define PCF_ASSUME_ALIGNED_AS(src, bytes) __builtin_assume(((ptrdiff_t)(src) % (ptrdiff_t)(bytes)) == 0)
#elif defined(__ICC) && __ICC > 1110
# define PCF_ASSUME_ALIGNED_AS(src, bytes) __assume_aligned(src, bytes)
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
/* broken in gcc <= 5.0.0 as of 2015-01-09 */
# define PCF_ASSUME_ALIGNED_AS(src, bytes) if ((ptrdiff_t)(src) % (ptrdiff_t)(bytes) != 0) __builtin_unreachable()
#elif defined(_MSC_VER) && _MSC_VER >= 1400
# define PCF_ASSUME_ALIGNED_AS(src, bytes) __assume(((ptrdiff_t)(src) % (ptrdiff_t)(bytes)) == 0)
#else
# define PCF_ASSUME_ALIGNED_AS(src, bytes)
#endif
#endif /* PCF_ASSUME_ALIGNED_AS */


/* assume passed expression */
#ifndef PCF_ASSUME
#if defined(__clang__) && ((__clang_major__ > 3) || (__clang_major__ == 3 && __clang_minor__ >= 6))
# define PCF_ASSUME(expr) __builtin_assume(expr)
#elif defined(__ICC) && __ICC > 1110
# define PCF_ASSUME(expr) __assume(expr)
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
# define PCF_ASSUME(expr) if ( ! (expr) ) __builtin_unreachable();
#elif defined(_MSC_VER) && _MSC_VER >= 1400
# define PCF_ASSUME(expr) __assume(expr)
#else
# define PCF_ASSUME(expr)
#endif
#endif /* PCF_ASSUME */


/* packed structure */
#ifndef PCF_PACKED_START
#if defined(__clang__) || defined(__GNUC__)
# define PCF_PACKED_START(x) x
# define PCF_PACKED_END __attribute__((__packed__)) ;
#elif defined(_MSC_VER)
# define PCF_PACKED_START(x) __pragma(pack(push, 1)) x
# define PCF_PACKED_END ; __pragma(pack(pop))
#else
# define PCF_PACKED_START(x) x
# define PCF_PACKED_END ;
#endif
#endif /* PCF_PACKED_START */


/* printf format check */
#ifndef PCF_PRINTF
#if defined(__clang__) || defined(__GNUC__)
# define PCF_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
# define PCF_PRINTF(fmt, args)
#endif
#endif /* PCF_PRINTF */


#define PCF_MIN(x, y) ((x) > (y) ? (y) : (x))
#define PCF_MAX(x, y) ((x) >= (y) ? (x) : (y))


/* suppress unused parameter warning */
#ifdef _MSC_VER
# define PCF_UNUSED(x)
#else /* not _MSC_VER */
# define PCF_UNUSED(x) (void)x;
#endif /* not _MSC_VER */


/* Windows workarounds */
#ifdef PCF_IS_WIN
# define fileno _fileno
# define fdopen _fdopen
# define setmode _setmode
# define get_osfhandle _get_osfhandle
# define open_osfhandle _open_osfhandle
# define O_TEXT _O_TEXT
# define O_WTEXT _O_WTEXT
# define O_U8TEXT _O_U8TEXT
# define O_U16TEXT _O_U16TEXT
# define O_BINARY _O_BINARY
# define O_RDONLY _O_RDONLY
#endif


/* MSVS workarounds */
#ifdef _MSC_VER
# define snprintf _snprintf
# define snwprintf _snwprintf
# define vsnprintf _vsnprintf
# define vsnwprintf _vsnwprintf
# define va_copy(dest, src) (dest = src)
#endif /* _MSC_VER */


/* Cygwin workaround */
#ifdef __CYGWIN__
# define O_U8TEXT 0x00040000
# define O_U16TEXT 0x00020000
#endif /* __CYGWIN__ */


#ifndef HAS_STRICMP
#define HAS_STRICMP
static inline int stricmp(const char * lhs, const char * rhs) {
	for (; *lhs && tolower(*lhs) == tolower(*rhs); lhs++, rhs++)
	if (*lhs == 0) return 0;
#ifdef __cplusplus
	return static_cast<unsigned char>(*lhs) - static_cast<unsigned char>(*rhs);
#else
	return *((const unsigned char *)lhs) - *((const unsigned char *)rhs);
#endif
}
#endif /* HAS_NO_STRICMP */


#endif /* __LIBPCF_TARGET_H__ */
