/**
 * @file Meta.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-02-21
 * @version 2023-06-06
 *
 * Type related helpers usually found in the C++11 standard template library but designed for
 * embedded environments.
 *
 * @remarks C++11 or newer is required.
 */
#ifndef __META_HPP__
#define __META_HPP__

#include <stddef.h>
#include <stdint.h>


#if __cplusplus < 201103L
#error This header needs at least a C++11 compliant compiler.
#endif


#if defined(__GNUC__) && !defined(_GNUC_VER)
#define _GNUC_VER ((__GNUC__ * 100) + __GNUC_MINOR__)
#define __META_HPP___GNUC_VER
#endif


#ifndef __has_feature
#define __has_feature(x) 0
#endif


namespace {


/* basics *****************************************************************************************/


typedef decltype(nullptr) nullptr_t;


/* integral_constant */
template <typename T, T Value>
struct integral_constant {
	static constexpr T value = Value;
	typedef T value_type;
	typedef integral_constant<T, Value> type;
};


/* true_type */
typedef integral_constant<bool, true> true_type;


/* false_type */
typedef integral_constant<bool, false> false_type;


/* yes_type */
typedef char yes_type;


/* no_type */
typedef long no_type;


/* enable_if */
template <bool B, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true, T> { typedef T type; };


/* disable_if */
template <bool B, typename T = void> struct disable_if { typedef T type; };
template <typename T> struct disable_if<true, T> {};


/* remove *****************************************************************************************/


/* remove_const */
template <typename T> struct remove_const { typedef T type; };
template <typename T> struct remove_const<const T> { typedef T type; };


/* remove_volatile */
template <typename T> struct remove_volatile { typedef T type; };
template <typename T> struct remove_volatile<volatile T> { typedef T type; };


/* remove_cv */
template <typename T> struct remove_cv { typedef typename remove_const<typename remove_volatile<T>::type>::type type; };


/* remove_reference */
template <typename T> struct remove_reference { typedef T type; };
template <typename T> struct remove_reference<T &> { typedef T type; };
template <typename T> struct remove_reference<T &&> { typedef T type; };


/* remove_extent */
template <typename T> struct remove_extent { typedef T type; };
template <typename T, size_t N> struct remove_extent<T[N]> { typedef T type; };
template <typename T> struct remove_extent<T[]> { typedef T type; };


/* remove_all_extents */
template <typename T> struct remove_all_extents { typedef T type; };
template <typename T, size_t N> struct remove_all_extents<T[N]> { typedef typename remove_all_extents<T>::type type; };
template <typename T> struct remove_all_extents<T[]> { typedef typename remove_all_extents<T>::type type; };


/* remove_pointer */
template <typename T> struct remove_pointer { typedef T type; };
template <typename T> struct remove_pointer<T *> { typedef T type; };
template <typename T> struct remove_pointer<const T *> { typedef const T type; };
template <typename T> struct remove_pointer<volatile T *> { typedef volatile T type; };
template <typename T> struct remove_pointer<volatile const T *> { typedef volatile const T type; };
template <typename T> struct remove_pointer<T * const> { typedef T type; };
template <typename T> struct remove_pointer<const T * const> { typedef const T type; };
template <typename T> struct remove_pointer<volatile T * const> { typedef volatile T type; };
template <typename T> struct remove_pointer<volatile const T * const> { typedef volatile const T type; };


/* add ********************************************************************************************/


/* add_const */
template <typename T> struct add_const { typedef const T type; };


/* add_volatile */
template <typename T> struct add_volatile { typedef volatile T type; };


/* add_cv */
template <typename T> struct add_cv { typedef typename add_const<typename add_volatile<T>::type>::type type; };


/* add_reference */
template <typename T> struct add_reference { typedef T & type; };
template <typename T> struct add_reference<T &> { typedef T & type; };


/* add_lvalue_reference */
template <typename T> struct add_lvalue_reference { typedef T & type; };
template <typename T> struct add_lvalue_reference<T &> { typedef T & type; };
template <> struct add_lvalue_reference<void> { typedef void type; };
template <> struct add_lvalue_reference<const void> { typedef const void type; };
template <> struct add_lvalue_reference<volatile void> { typedef volatile void type; };
template <> struct add_lvalue_reference<volatile const void> { typedef volatile const void type; };


/* add_rvalue_reference */
template <typename T> struct add_rvalue_reference { typedef T && type; };
template <typename T> struct add_rvalue_reference<T &> { typedef T & type; };
template <> struct add_rvalue_reference<void> { typedef void type; };
template <> struct add_rvalue_reference<const void> { typedef const void type; };
template <> struct add_rvalue_reference<volatile void> { typedef volatile void type; };
template <> struct add_rvalue_reference<volatile const void> { typedef volatile const void type; };


/* add_pointer */
template <typename T> struct add_pointer { typedef typename remove_reference<T>::type * type; };


/* is *********************************************************************************************/


/* is_same */
template <typename, typename> struct is_same : false_type {};
template <typename T> struct is_same<T, T> : true_type {};


/* is_const */
template <typename> struct is_const : false_type {};
template <typename T> struct is_const<const T> : true_type {};


/* is_volatile */
template <typename> struct is_volatile : false_type {};
template <typename T> struct is_volatile<volatile T> : true_type {};


/* is_void */
template <typename T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};


/* is_null_pointer */
template <typename T> struct is_null_pointer : is_same<nullptr_t, typename remove_cv<T>::type> {};


/* is_array */
template <typename> struct is_array : false_type {};
template <typename T, size_t N> struct is_array<T[N]> : true_type {};
template <typename T> struct is_array<T[]> : true_type {};


/* is_pointer */
template <typename T> struct is_pointer : false_type {};
template <typename T> struct is_pointer<T *> : true_type {};


/* is_reference */
template <typename T> struct is_reference : false_type {};
template <typename T> struct is_reference<T &> : true_type {};


/* is_lvalue_reference */
template <typename T> struct is_lvalue_reference : false_type {};
template <typename T> struct is_lvalue_reference<T &> : true_type {};


/* is_integral */
template <typename T> struct is_integral : false_type {};
template <typename T> struct is_integral<const T> : is_integral<T> {};
template <typename T> struct is_integral<volatile const T> : is_integral<T> {};
template <typename T> struct is_integral<volatile T> : is_integral<T> {};
template <> struct is_integral<bool> : true_type {};
template <> struct is_integral<unsigned char> : true_type {};
template <> struct is_integral<signed char> : true_type {};
template <> struct is_integral<unsigned short> : true_type {};
template <> struct is_integral<signed short> : true_type {};
template <> struct is_integral<unsigned int> : true_type {};
template <> struct is_integral<signed int> : true_type {};
template <> struct is_integral<unsigned long> : true_type {};
template <> struct is_integral<signed long> : true_type {};
template <> struct is_integral<unsigned long long> : true_type {};
template <> struct is_integral<signed long long> : true_type {};
template <> struct is_integral<wchar_t> : true_type {};


/* is_unsigned */
template <typename T> struct is_unsigned : false_type {};
template <typename T> struct is_unsigned<const T> : is_unsigned<T> {};
template <typename T> struct is_unsigned<volatile T> : is_unsigned<T> {};
template <typename T> struct is_unsigned<volatile const T> : is_unsigned<T> {};
template <> struct is_unsigned<bool> : true_type {};
template <> struct is_unsigned<char> : integral_constant<bool, (char(255) > 0)> {};
template <> struct is_unsigned<unsigned char> : true_type {};
template <> struct is_unsigned<unsigned short> : true_type {};
template <> struct is_unsigned<unsigned int> : true_type {};
template <> struct is_unsigned<unsigned long> : true_type {};
template <> struct is_unsigned<unsigned long long> : true_type {};
template <> struct is_unsigned<wchar_t> :integral_constant<bool, (wchar_t(65535) > 0)> {};


/* is_signed */
template <typename T> struct is_signed : false_type {};
template <typename T> struct is_signed<const T> : is_signed<T> {};
template <typename T> struct is_signed<volatile T> : is_signed<T> {};
template <typename T> struct is_signed<volatile const T> : is_signed<T> {};
template <> struct is_signed<char> : integral_constant<bool, (char(255) < 0)> {};
template <> struct is_signed<signed char> : true_type {};
template <> struct is_signed<short> : true_type {};
template <> struct is_signed<int> : true_type {};
template <> struct is_signed<long> : true_type {};
template <> struct is_signed<long long> : true_type {};
template <> struct is_signed<float> : true_type{};
template <> struct is_signed<double> : true_type{};
template <> struct is_signed<long double> : true_type{};
template <> struct is_signed<wchar_t> :integral_constant<bool, (wchar_t(65535) < 0)> {};


/* is_floating_point */
template <typename T> struct is_floating_point : false_type {};
template <typename T> struct is_floating_point<const T> : is_floating_point<T> {};
template <typename T> struct is_floating_point<volatile const T> : is_floating_point<T> {};
template <typename T> struct is_floating_point<volatile T> : is_floating_point<T> {};
template <> struct is_floating_point<float> : true_type {};
template <> struct is_floating_point<double> : true_type {};
template <> struct is_floating_point<long double> : true_type {};


/* is_arithmetic */
template <typename T> struct is_arithmetic : integral_constant<bool, is_integral<T>::value || is_floating_point<T>::value> {};


/* is_fundamental */
template <typename T> struct is_fundamental : integral_constant<bool, is_arithmetic<T>::value || is_void<T>::value || is_same<void *, typename remove_cv<T>::type>::value> {};


/* is_compound */
template <typename T> struct is_compound : integral_constant<bool, !is_fundamental<T>::value> {};


/* is_abstract */
#if __has_feature(is_abstract) || (_GNUC_VER >= 403)
template <typename T> struct is_abstract : integral_constant<bool, __is_abstract(T)> {};
#endif


/* is_empty */
#if __has_feature(is_empty) || (_GNUC_VER >= 403)
template <typename T> struct is_empty : integral_constant<bool, __is_empty(T)> {};
#endif


/* is_enum */
#if __has_feature(is_enum) || (_GNUC_VER >= 403)
template <typename T> struct is_enum : integral_constant<bool, __is_enum(T)> {};
#endif


/* is_union */
#if __has_feature(is_union) || (_GNUC_VER >= 403)
template <typename T> struct is_union : integral_constant<bool, __is_union(T)> {};
#endif


/* is_polymorphic */
#if __has_feature(is_polymorphic) || (_GNUC_VER >= 403)
template <typename T> struct is_polymorphic : integral_constant<bool, __is_polymorphic(T)> {};
#endif


/* is_trivial */
#if __has_feature(is_trivial) || (_GNUC_VER >= 403)
template <typename T> struct is_trivial : integral_constant<bool, __is_trivial(T)> {};
#endif


/* is_pod */
#if __has_feature(is_pod) || (_GNUC_VER >= 403)
template <typename T> struct is_pod : integral_constant<bool, __is_pod(T)> {};
#else
template <typename T> struct is_pod : integral_constant<bool, is_fundamental<T>::value || is_pointer<T>::value> {};
#endif


/* is_trivially_constructible */
template <typename T> struct is_trivially_constructible : is_pod<T> {};


/* is_trivially_copy_constructible */
template <typename T> struct is_trivially_copy_constructible : is_pod<T> {};


/* is_trivially_destructible */
template <typename T> struct is_trivially_destructible : is_pod<T> {};


/* is_trivially_copy_assignable */
template <typename T> struct is_trivially_copy_assignable : is_pod<T> {};


/* is_class */
#if __has_feature(is_class) || (_GNUC_VER >= 403)
template <typename T> struct is_class : integral_constant<bool, __is_class(T)> {};
#else
template <typename T>
class is_class {
private:
	template <typename C> static yes_type & test(int C::*);
	template <typename C> static no_type & test(...);
public:
	static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);
};
#endif


/* is_base_of */
#if __has_feature(is_base_of) || (_GNUC_VER >= 403)
template <typename Base, typename Derived> struct is_base_of : integral_constant<bool, __is_base_of(Base, Derived)> {};
#else
template <typename Base, typename Derived, const bool IsFundamental = (is_fundamental<Base>::value || is_fundamental<Derived>::value)>
class is_base_of {
private:
	template <typename T> struct Dummy {};
	struct Child : Derived, Dummy<int> {};
	static Base * test(Base *);
	template <typename T> static char test(Dummy<T> *);
public:
	static constexpr bool value = (sizeof(test(static_cast<Child *>(0))) == sizeof(Base *));
};

template <typename Base, typename Derived>
struct is_base_of<Base, Derived, true> {
	static constexpr bool value = false;
};
#endif

/* is_convertible */
template <typename From, typename To>
class is_convertible {
private:
	template <typename T> static yes_type test(T);
	template <typename T> static no_type test(...);
	template <typename T> static T & create();
public:
	static constexpr bool value = sizeof(test<To>(create<From>())) == sizeof(yes_type);
};


/* is_function */
template <typename> struct is_function : false_type {};
template <typename R, typename ...Args> struct is_function<R(Args...)> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...)> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) const> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) volatile> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) volatile const> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) const> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) volatile> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) volatile const> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) const &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) volatile &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) volatile const &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) const &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) volatile &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) volatile const &> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) const &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) volatile &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args...) volatile const &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) const &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) volatile &&> : true_type {};
template <typename R, typename ...Args> struct is_function<R(Args..., ...) volatile const &&> : true_type {};


/* is_member_pointer */
template <typename R> struct is_member_pointer_helper : false_type {};
template <typename R, typename T> struct is_member_pointer_helper<R T::*> : true_type {};
template <typename R> struct is_member_pointer : is_member_pointer_helper<typename remove_cv<R>::type> {};


/* is_member_function_pointer */
template <typename R> struct is_member_function_pointer_helper : false_type {};
template <typename R, typename T> struct is_member_function_pointer_helper<R T::*> : is_function<R> {};
template <typename R> struct is_member_function_pointer : is_member_function_pointer_helper<typename remove_cv<R>::type> {};


/* is_member_object_pointer */
template <typename T> struct is_member_object_pointer : integral_constant<bool, is_member_pointer<T>::value && !is_member_function_pointer<T>::value> {};


/* is_scalar */
#if __has_feature(is_enum) || (_GNUC_VER >= 403)
template <typename T > struct is_scalar : integral_constant<bool, is_arithmetic<T>::value || is_enum<T>::value || is_pointer<T>::value || is_member_pointer<T>::value || is_null_pointer<T>::value> {};
#else
template <typename T > struct is_scalar : integral_constant<bool, is_arithmetic<T>::value || is_pointer<T>::value || is_member_pointer<T>::value || is_null_pointer<T>::value> {};
#endif


/* utility ****************************************************************************************/


/* reference_wrapper */
template <typename T>
class reference_wrapper {
private:
	T * ptr;
public:
	typedef T type;

	reference_wrapper(T & ref) noexcept : ptr(&ref) {}
	reference_wrapper(T &&) = delete;
	reference_wrapper(const reference_wrapper &) noexcept = default;
	reference_wrapper & operator= (const reference_wrapper & x) noexcept = default;
	operator T & () const noexcept { return *ptr; }
	T & get() const noexcept { return *ptr; }
};


/* ref() */
template <typename T> reference_wrapper<T> ref(T & t) noexcept;
template <typename T> reference_wrapper<T> ref(reference_wrapper<T> val) noexcept;
template <typename T> void ref(const T &&) = delete;


/* cref() */
template <typename T> reference_wrapper<const T> cref(const T & val) noexcept;
template <typename T> reference_wrapper<const T> cref(reference_wrapper<T> val) noexcept;
template <typename T> void cref(const T &&) = delete;


/* conditional */
template <bool B, typename T, typename F>  struct conditional { typedef T type; };
template <typename T, typename F> struct conditional<false, T, F> { typedef F type; };


/* conditional_integral_constant */
template <bool B, typename T, T TrueValue, T FalseValue>
struct conditional_integral_constant;

template <typename T, T TrueValue, T FalseValue>
struct conditional_integral_constant<true, T, TrueValue, FalseValue> {
	static_assert(is_integral<T>::value, "Not an integral type.");
	static const T value = TrueValue;
};

template <typename T, T TrueValue, T FalseValue>
struct conditional_integral_constant<false, T, TrueValue, FalseValue> {
	static_assert(is_integral<T>::value, "Not an integral type.");
	static const T value = FalseValue;
};


/* is_any_of */
template <typename T, typename T0, typename ...Ts>
struct is_any_of : integral_constant<bool, conditional<is_same<T, T0>::value, true_type, is_any_of<T, Ts...> >::type::value> {};
template <typename T, typename T0> struct is_any_of<T, T0> : is_same<T, T0>::type {};


/* decay */
template <typename T>
class decay {
private:
	template <typename U, bool Array, bool Function> struct DecayImpl { typedef typename remove_cv<U>::type type; };
	template <typename U> struct DecayImpl<U, true, false> { typedef typename remove_extent<U>::type * type; };
	template <typename U> struct DecayImpl<U, false, true> { typedef U * type; };
	typedef typename remove_reference<T>::type NoneRefT;
public:
	typedef typename DecayImpl<NoneRefT, is_array<NoneRefT>::value, is_function<NoneRefT>::value>::type type;
};


/* member_function_pointer_class */
template <typename Sig> struct member_function_pointer_class;
template <typename F, typename T> struct member_function_pointer_class<F T::*> { typedef T type; };


/* member_function_pointer_function */
template <typename Sig> struct member_function_pointer_function;
template <typename F, typename T> struct member_function_pointer_function<F T::*> { typedef F type; };


/* type_identity */
template <typename T> struct type_identity { typedef T type; };


/* parameter_pack */
template <typename ...> struct parameter_pack {};


/* declval() */
template <typename T> typename add_rvalue_reference<T>::type declval();


/* integer_sequence */
template <typename T, T... N>
struct integer_sequence {
	typedef T value_type;
	static_assert(is_integral<T>::value, "integer_sequence can only be instantiated with an integral type");
	static constexpr size_t size() { return (sizeof...(N)); }
};


/* index_sequence */
template <size_t... N> struct index_sequence : integer_sequence<size_t, N...> {};


/* make_integer_sequence */
template <typename T, size_t N, T ...Rest> struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Rest...> {};
template <typename T, T ...Rest> struct make_integer_sequence<T, 0, Rest...> { typedef integer_sequence<T, Rest...> type; };


/* make_index_sequence */
template <size_t N, size_t ...Rest> struct make_index_sequence : make_index_sequence<N - 1, N - 1, Rest...> {};
template <size_t ...Rest> struct make_index_sequence<0, Rest...> { typedef index_sequence<Rest...> type; };


/* make_integer_repetition */
template <typename T, T i, size_t N, size_t ...Rest> struct make_integer_repetition : make_integer_repetition<T, i, N - 1, i, Rest...> {};
template <typename T, T i, size_t ...Rest> struct make_integer_repetition<T, i, 0, Rest...> { typedef integer_sequence<T, Rest...> type; };


/* make_index_repetition */
template <size_t i, size_t N, size_t ...Rest> struct make_index_repetition : make_index_repetition<i, N - 1, i, Rest...> {};
template <size_t i, size_t ...Rest> struct make_index_repetition<i, 0, Rest...> { typedef index_sequence<Rest...> type; };


/* result_of */
template <typename Sig> struct result_of;
template <typename R, typename ...Args> struct result_of<R(Args...)> { typedef decltype(declval<R>()(declval<Args>()...)) type; };


/* return_type_of */
template <typename Sig> struct return_type_of;
template <typename R, typename ...Args> struct return_type_of<R(Args...)> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...)> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) const> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) volatile> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) volatile const> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) const> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) volatile> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) volatile const> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) const &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) volatile &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) volatile const &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) const &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) volatile &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) volatile const &> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) const &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) volatile &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args...) volatile const &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) const &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) volatile &&> : type_identity<R> {};
template <typename R, typename ...Args> struct return_type_of<R(Args..., ...) volatile const &&> : type_identity<R> {};
template <typename F, typename T> struct return_type_of<F T::*> : return_type_of<F> {};
template <typename F, typename T> struct return_type_of<F T::* const> : return_type_of<F> {};
template <typename F, typename T> struct return_type_of<F T::* volatile> : return_type_of<F> {};
template <typename F, typename T> struct return_type_of<F T::* volatile const> : return_type_of<F> {};


/* arguments_of */
template <typename Sig> struct arguments_of;
template <typename R, typename ...Args> struct arguments_of<R(Args...)> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(Args...) const> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(Args...) volatile> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(Args...) volatile const> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(*)(Args...)> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(* const)(Args...)> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(* volatile)(Args...)> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(* volatile const)(Args...)> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(&)(Args...)> { typedef parameter_pack<Args...> type; };
template <typename R, typename ...Args> struct arguments_of<R(&&)(Args...)> { typedef parameter_pack<Args...> type; };
template <typename F, typename T> struct arguments_of<F T::*> : arguments_of<F> {};
template <typename F, typename T> struct arguments_of<F T::* const> : arguments_of<F> {};
template <typename F, typename T> struct arguments_of<F T::* volatile> : arguments_of<F> {};
template <typename F, typename T> struct arguments_of<F T::* volatile const> : arguments_of<F> {};


/* parameter_pack_size */
template <typename T> struct parameter_pack_size;
template <typename ...Args> struct parameter_pack_size<parameter_pack<Args...>> : integral_constant<size_t, sizeof...(Args)> {};


/* flat_parameter_pack */
/* e.g. parameter_pack<parameter_pack<...>, parameter_pack<...>> to parameter_pack<...> */
template <typename T, typename ...> struct flat_parameter_pack { typedef T type; };
template <typename T> struct flat_parameter_pack<parameter_pack<T>>  { typedef T type; };
template <typename ...T0, typename ...T1, typename ...Args>
struct flat_parameter_pack<parameter_pack<parameter_pack<T0...>, parameter_pack<T1...>, Args...>>
	: public flat_parameter_pack<parameter_pack<parameter_pack<T0..., T1...>, Args...>> {};


/* map_parameter_pack */
/* e.g. for M: template <typename T> struct M { typedef parameter_pack<T> type; }; */
template <template <typename> class M, typename ...Args>
struct mapped_parameter_pack { typedef typename flat_parameter_pack<parameter_pack<typename M<Args>::type...>>::type type; };


/* has_function_x */
#define DEF_HAS_FUNCTION(x) \
	template <typename ...Args> \
	class has_function_##x { \
		template <typename T> struct as_true_type : true_type {}; \
		template <typename ...Ts> static auto internal_##x(const int, Ts... args) -> has_function_##x::as_true_type<decltype(x(args...))> { return true_type(); } \
		template <typename ...Ts> static auto internal_##x(const float, Ts...) -> false_type { return false_type(); } \
	public: \
		static constexpr bool value = decltype(has_function_##x::internal_##x(0, Args()...))::value; \
	};


/* has_member_x */
#define DEF_HAS_MEMBER(x) \
	template <typename T> \
	class has_member_##x { \
		template <typename U> static yes_type test(decltype(&U::x)); \
		template <typename U> static no_type test(...); \
	public: \
		static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type); \
	};


/* move() */
template <typename T> typename remove_reference<T>::type && move(T & val) { return static_cast<typename remove_reference<T>::type &&>(val); }
template <typename T> typename remove_reference<T>::type && move(T && val) { return static_cast<typename remove_reference<T>::type &&>(val); }


/* forward() */
template <typename T> T && forward(typename remove_reference<T>::type & val) noexcept {
	return static_cast<T &&>(val);
}
template <typename T> T && forward(typename remove_reference<T>::type && val) noexcept {
	static_assert(!is_lvalue_reference<T>::value, "rvalue cannot be forwarded as an lvalue.");
	return static_cast<T &&>(val);
}


/* tuple */
struct tuple_detail {
	template <size_t i, typename T>
	struct leaf {
		T value;
		leaf() {}
		explicit leaf(T val): value(val) {}
	};

	template <size_t i, typename ...Args> struct impl;
	template <size_t i> struct impl<i>{};

	template <size_t i, typename T0, typename ...Args>
	struct impl<i, T0, Args...> : public leaf<i, T0>, public impl<i + 1, Args...> {
		impl() {}

		explicit impl(T0 a0, Args... args):
			leaf<i, T0>(forward<T0>(a0)), impl<i + 1, Args...>(forward<Args>(args)...)
		{}

		constexpr size_t size() const {
			return sizeof...(Args) + 1;
		}
	};

	template <typename T> struct unwrap_refwrapper { using type = T; };
	template <typename T> struct unwrap_refwrapper<reference_wrapper<T>> { using type = T &; };
	template <typename T> struct special_decay { typename unwrap_refwrapper<typename decay<T>::type>::type type; };
};

template <typename ...Args>
using tuple = tuple_detail::impl<0, Args...>;

template <typename T> struct tuple_size;
template <typename ...Args> struct tuple_size<tuple<Args...>> : integral_constant<size_t, sizeof...(Args)> {};
template <typename ...Args> struct tuple_size<const tuple<Args...>> : integral_constant<size_t, sizeof...(Args)> {};
template <typename ...Args> struct tuple_size<volatile tuple<Args...>> : integral_constant<size_t, sizeof...(Args)> {};
template <typename ...Args> struct tuple_size<volatile const tuple<Args...>> : integral_constant<size_t, sizeof...(Args)> {};

template <size_t i, typename T> struct tuple_element;
template <size_t i, typename T0, typename ...Args> struct tuple_element<i, tuple<T0, Args...>> : tuple_element<i - 1, tuple<Args...>> {};
template <typename T0, typename ...Args> struct tuple_element<0, tuple<T0, Args...>> { typedef T0 type; };

template <size_t i, typename T0, typename ...Args>
T0 & get(tuple_detail::impl<i, T0, Args...> & val) {
	return val.tuple_detail::template leaf<i, T0>::value;
}

template <size_t i, typename T0, typename ...Args>
const T0 & get(const tuple_detail::impl<i, T0, Args...> & val) {
	return val.tuple_detail::template leaf<i, T0>::value;
}

template <typename ...Args>
tuple<typename tuple_detail::special_decay<Args>::type...> make_tuple(Args && ...args) {
    return tuple<typename tuple_detail::special_decay<Args>::type...>(forward<Args>(args)...);
}

template <typename T> struct tuple_from;
template <typename ...Args> struct tuple_from<parameter_pack<Args...>> { typedef tuple<Args...> type; };


/* function_n */
template <typename Sig, size_t Size>
class function_n;

template <typename R, typename ...Args, size_t Size>
class function_n<R(Args...), Size> {
private:
	struct vtable_t {
		void (* mover)(void * src, void * dest);
		void (* destroyer)(void *);
		R (* invoker)(const void * o, Args && ...args);

		template <typename T>
		static const vtable_t * get() {
			static const vtable_t vtable = {
				[](void * src, void * dest) { new (dest) T(move(*static_cast<T *>(src))); },
				[](void * o) { static_cast<T *>(o)->~T(); },
				[](const void * o, Args && ...args) -> R { return (*static_cast<const T *>(o))(forward<Args>(args)...); }
			};
			return &vtable;
		}
	};

	const vtable_t * vtable = NULL;
	char data[Size];
public:
	function_n() {}

	template <
		typename F,
		typename DF = typename decay<F>::type,
		typename enable_if<!is_same<DF, function_n>::vale>::type * = NULL,
		typename enable_if<typename is_convertible<typename result_of<const DF & (Args...)>::type, F>::type{}>::type * = NULL
	>
	function_n(F && f):
		vtable(vtable_t::template get<DF>())
	{
		static_assert(sizeof(DF) <= Size, "Object is too large.");
		new (&data) DF(forward<F>(f));
	}

	function_n(function_n && o):
		vtable(o.vtable)
	{
		if (vtable != NULL) vtable->mover(&o.data, &data);
	}

	~function_n() {
		if (vtable != NULL) vtable->destroyer(&data);
	}

	function_n & operator= (function_n && o) {
		this->~function_n();
		new (this) function_n(move(o));
		return *this;
	}

	explicit operator bool() const {
		return vtable != NULL;
	}

	R operator() (Args... args) const {
		return vtable->invoker(&data, forward<Args>(args)...);
	}
};


/* swap() */
template <typename T>
void swap(T & a, T & b) {
	T tmp = move(a);
	a = move(b);
	b = move(tmp);
}


/* reverse() */
template <typename BidirectionalIt>
void reverse(BidirectionalIt first, BidirectionalIt last) {
    while ((first != last) && (first != --last)) {
        swap(*first++, *last);
    }
}


} /* anonymous namespace */


#ifdef __META_HPP___GNUC_VER
#undef _GNUC_VER
#endif


#endif /* __META_HPP__ */
