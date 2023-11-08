/**
 * @file ScopeExit.hpp
 * @author Daniel Starke
 * @date 2019-10-04
 * @version 2023-10-03
 */
#ifndef __PCF_SCOPEEXIT_HPP__
#define __PCF_SCOPEEXIT_HPP__

#include <utility>


/**
 * Helper class to initiate the call at scope end.
 *
 * @tparam Fn - function type
 */
template <typename Fn>
class ScopeExit {
private:
	Fn fn;
public:
	/**
	 * Constructor
	 *
	 * @param[in,out] f - moved function to execute on destruction
	 */
	explicit inline ScopeExit(Fn && f):
		fn(std::forward<Fn>(f))
	{}

	/** Destructor. */
	inline ~ScopeExit() {
		this->fn();
	}
};


/**
 * Executes the given function at the end of the scope of the returned object.
 *
 * @param[in] f - function to call
 * @return RAII object for scope exit handling
 * @tparam Fn - function type
 */
template <typename Fn>
inline ScopeExit<Fn> makeScopeExit(Fn && f) {
	return ScopeExit<Fn>(std::forward<Fn>(f));
}


#endif /* __PCF_SCOPEEXIT_HPP__ */
