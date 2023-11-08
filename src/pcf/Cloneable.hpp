/**
 * @file Cloneable.hpp
 * @author Daniel Starke
 * @date 2019-10-04
 * @version 2023-10-03
 */
#ifndef __PCF_CLONEABLE_HPP__
#define __PCF_CLONEABLE_HPP__

#include <utility>


/**
 * Provides the cloneable base interface.
 *
 * @tparam Base - base class to provide the cloneable interface for
 */
template <typename Base>
class CloneableInterface {
public:
	/** Destructor. */
	virtual ~CloneableInterface() {}

	/**
	 * Creates a copy of the current instance.
	 *
	 * @return cloned instance
	 */
	virtual Base * clone() const = 0;
};


/**
 * Provides the cloneable implementation.
 *
 * @tparam Derived - class to implement the cloneable interface for
 * @tparam Base - class to derive from
 */
template <typename Derived, typename Base>
class Cloneable : public Base {
public:
	/**
	 * Constructor
	 *
	 * @param[in] args - base constructor argument list
	 */
	template <typename... Args>
	explicit inline Cloneable(Args &&... args):
		Base(std::forward<Args...>(args)...)
	{}

	/** Destructor. */
	virtual ~Cloneable() {}

	/**
	 * Creates a copy of the current instance.
	 *
	 * @return cloned instance
	 */
	virtual Base * clone() const {
		return new Derived(static_cast<const Derived &>(*this));
	}
};


#endif /* __PCF_CLONEABLE_HPP__ */
