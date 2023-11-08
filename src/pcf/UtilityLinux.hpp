/**
 * @file UtilityLinux.hpp
 * @author Daniel Starke
 * @date 2020-01-26
 * @version 2020-01-26
 */
#ifndef __PCF_UTILITYLINUX_HPP__
#define __PCF_UTILITYLIUNX_HPP__


#include <vkm-periphery/Meta.hpp>
extern "C" {
#include <errno.h>
#include <libpcf/target.h>
}


/**
 * Helper function to retry a given function on EINTR errors.
 *
 * @param[in] fn - function to call
 * @param[in,out] args - function arguments
 * @return the result of the function
 * @tparam R - function return type
 * @tparam ...Args - function argument types
 *
 */
template <typename Fn, typename ...Args, typename R = typename return_type_of<Fn>::type>
static inline R xEINTR(Fn & fn, Args... args) {
	R res;
	do {
		errno = 0;
		res = fn(args...);
	} while (errno == EINTR);
	return res;
}


#endif /* __PCF_UTILITYLINUX_HPP__ */
