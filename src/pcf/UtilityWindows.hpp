/**
 * @file UtilityWindows.hpp
 * @author Daniel Starke
 * @date 2023-11-25
 * @version 2023-12-04
 */
#ifndef __PCF_UTILITYWINDOWS_HPP__
#define __PCF_UTILITYWINDOWS_HPP__

#include <vkm-periphery/Meta.hpp>
#include <windows.h>
#include <unknwn.h>


namespace Microsoft {
namespace WRL {
namespace Details {


inline void DECLSPEC_NORETURN RaiseException(HRESULT hr, DWORD flags = EXCEPTION_NONCONTINUABLE) noexcept {
	::RaiseException(DWORD(hr), flags, 0, NULL);
}


template <bool b, typename T = void>
struct EnableIf {};


template <typename T>
struct EnableIf<true, T> {
	typedef T type;
};


template <typename T>
class ComPtrRefBase {
protected:
	T * ptr_;
public:
	typedef typename T::InterfaceType InterfaceType;

	inline operator IUnknown **() const noexcept {
		static_assert(is_base_of<IUnknown, InterfaceType>::value, "invalid cast");
		return reinterpret_cast<IUnknown **>(this->ptr_->ReleaseAndGetAddressOf());
	}
};


template <typename T>
class ComPtrRef : public ComPtrRefBase<T> {
public:
	typedef typename T::InterfaceType InterfaceType;

	inline ComPtrRef(T * ptr) noexcept {
		ComPtrRefBase<T>::ptr_ = ptr;
	}

	inline operator void **() const noexcept {
		return reinterpret_cast<void**>(ComPtrRefBase<T>::ptr_->ReleaseAndGetAddressOf());
	}

	inline operator T *() noexcept {
		*(this->ComPtrRefBase<T>::ptr_) = nullptr;
		return this->ComPtrRefBase<T>::ptr_;
	}

	inline operator InterfaceType ** () noexcept {
		return this->ComPtrRefBase<T>::ptr_->ReleaseAndGetAddressOf();
	}

	inline InterfaceType * operator* () noexcept {
		return this->ComPtrRefBase<T>::ptr_->Get();
	}

	inline InterfaceType * const * GetAddressOf() const noexcept {
		return this->ComPtrRefBase<T>::ptr_->GetAddressOf();
	}

	inline InterfaceType ** ReleaseAndGetAddressOf() noexcept {
		return this->ComPtrRefBase<T>::ptr_->ReleaseAndGetAddressOf();
	}
};


} /* namespace Details */


/**
 * Smart pointer for OLE/COM objects.
 *
 * @tparam T - interface type
 * @see https://learn.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class
 */
template <typename T>
class ComPtr {
	template <typename U> friend class ComPtr;
protected:
	T * ptr_; /** Pointer to the managed object of the specialized interface. */
public:
	typedef T InterfaceType; /**< Represented interface. */

	/** Constructor. */
	inline ComPtr() noexcept:
		ptr_(NULL)
	{}

	/** Constructor. */
	inline ComPtr(decltype(nullptr)) noexcept:
		ptr_(NULL)
	{}

	/**
	 * Constructor.
	 *
	 * @param[in] p - object pointer
	 */
	template <typename U>
	inline ComPtr(U * p) noexcept:
		ptr_(p)
	{
		this->InternalAddRef();
	}

	/**
	 * Copy constructor.
	 *
	 * @param[in] o - object to copy
	 */
	template <typename U>
	inline ComPtr(const ComPtr<U> & o) noexcept:
		ptr_(o.Get())
	{
		this->InternalAddRef();
	}

	/**
	 * Copy constructor.
	 *
	 * @param[in] o - object to copy
	 */
	inline ComPtr(const ComPtr & o) noexcept:
		ptr_(o.ptr_)
	{
		this->InternalAddRef();
	}

	/**
	 * Move constructor.
	 *
	 * @param[in,out] o - object to move
	 */
	template <typename U>
	inline ComPtr(ComPtr<U> && o) noexcept:
		ptr_(o.Detach())
	{}

	/** Destructor. */
	inline ~ComPtr() noexcept {
		this->InternalRelease();
	}

	/**
	 * Assignment operator.
	 *
	 * @return `*this`
	 */
	inline ComPtr & operator= (decltype(nullptr)) noexcept {
		this->InternalRelease();
		return *this;
	}

	/**
	 * Assignment operator.
	 *
	 * @param[in] p - pointer to assign
	 * @return `*this`
	 */
	template <typename U>
	inline ComPtr & operator= (U * p) noexcept {
		if (this->ptr_ != p) {
			this->InternalRelease();
			this->ptr_ = p;
			this->InternalAddRef();
		}
		return *this;
	}

	/**
	 * Assignment operator.
	 *
	 * @param[in] o - object to assign
	 * @return `*this`
	 */
	template <typename U>
	inline ComPtr & operator= (const ComPtr<U> & o) noexcept {
		ComPtr(o).Swap(*this);
		return *this;
	}

	/**
	 * Assignment operator.
	 *
	 * @param[in] o - object to assign
	 * @return `*this`
	 */
	inline ComPtr & operator= (const ComPtr	& o) noexcept {
		if (this->ptr_ != o.ptr_) {
			ComPtr(o).Swap(*this);
		}
		return *this;
	}

	/**
	 * Move operator.
	 *
	 * @param[in,out] o - object to move
	 * @return `*this`
	 */
	template <typename U>
	inline ComPtr & operator= (ComPtr<U> && o) noexcept {
		ComPtr(o).Swap(*this);
		return *this;
	}

	/**
	 * Swaps the contained pointers.
	 *
	 * @param[in,out] o - object to swap with
	 */
	inline void Swap(ComPtr & o) noexcept {
		T * tmp = this->ptr_;
		this->ptr_ = o.ptr_;
		o.ptr_ = tmp;
	}

	/**
	 * Swaps the contained pointers.
	 *
	 * @param[in,out] o - object to swap with
	 */
	inline void Swap(ComPtr && o) noexcept {
		T * tmp = this->ptr_;
		this->ptr_ = o.ptr_;
		o.ptr_ = tmp;
	}

	/**
	 * Returns the pointer to the managed interface.
	 *
	 * @return interface pointer
	 */
	inline T * Get() const noexcept {
		return this->ptr_;
	}

	/**
	 * Implicit pointer cast.
	 *
	 * @return interface pointer
	 */
	inline operator T*() const noexcept {
		return this->ptr_;
	}

	/**
	 * Indirection operation.
	 *
	 * @return interface pointer
	 */
	inline T * operator-> () const noexcept {
		return this->ptr_;
	}

	/**
	 * Dereference operator.
	 *
	 * @return object pointing to this object
	 */
	inline Details::ComPtrRef< ComPtr<T> > operator& () noexcept {
		return Details::ComPtrRef< ComPtr<T> >(this);
	}

	/**
	 * Dereference operator.
	 *
	 * @return object pointing to this object
	 */
	inline const Details::ComPtrRef< const ComPtr<T> > operator& () const noexcept {
		return Details::ComPtrRef< const ComPtr<T> >(this);
	}

	/**
	 * Returns a pointer to the stored pointer.
	 *
	 * @return pointer to this pointer
	 */
	inline T * const * GetAddressOf() const noexcept {
		return &(this->ptr_);
	}

	/**
	 * Returns a pointer to the stored pointer.
	 *
	 * @return pointer to this pointer
	 */
	inline T ** GetAddressOf() noexcept {
		return &(this->ptr_);
	}

	/**
	 * Returns a pointer to the stored pointer.
	 * Also releases the taken reference.
	 *
	 * @return pointer to this pointer
	 */
	inline T ** ReleaseAndGetAddressOf() noexcept {
		this->InternalRelease();
		return &(this->ptr_);
	}

	/**
	 * Attaches a already acquired interface pointer.
	 *
	 * @param[in,out] p - interface pointer to attach
	 */
	inline void Attach(T * p) noexcept {
		if (ptr_ != p) {
			this->InternalRelease();
			this->ptr_ = p;
			/* assume already acquired */
		}
	}

	/**
	 * Detaches the included interface pointer.
	 * This does not decrease the reference count.
	 *
	 * @return detached interface pointer
	 */
	inline T * Detach() noexcept {
		T * res = this->ptr_;
		this->ptr_ = nullptr;
		return res;
	}

	/**
	 * Resets the object. No interface pointer is managed hereafter.
	 *
	 * @return reference count
	 */
	inline ULONG Reset() {
		return this->InternalRelease();
	}

	/**
	 * Copies the interface pointer to the passed pointer.
	 *
	 * @param[out] p - pointer receiving the copy
	 * @return same as `QueryInterface`
	 */
	template <typename U>
	inline HRESULT CopyTo(U ** p) const noexcept {
		return this->ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void **>(p));
	}

	/**
	 * Copies the interface pointer to the passed pointer.
	 *
	 * @param[in] riid - referred interface ID
	 * @param[out] p - pointer receiving the copy
	 * @return same as `QueryInterface`
	 */
	inline HRESULT CopyTo(REFIID riid, void ** p) const noexcept {
		return this->ptr_->QueryInterface(riid, p);
	}

	/**
	 * Casts the pointed interface to another one.
	 *
	 * @param[in,out] o - object to receive the casted interface pointer
	 * @return same as `QueryInterface`
	 */
	template <typename U>
	inline HRESULT As(Details::ComPtrRef< ComPtr<U> > o) const noexcept {
		return this->ptr_->QueryInterface(__uuidof(U), o);
	}

	/**
	 * Casts the pointed interface to another one.
	 *
	 * @param[in,out] o - object to receive the casted interface pointer
	 * @return same as `QueryInterface`
	 */
	template<typename U>
	inline HRESULT As(ComPtr<U> * o) const noexcept {
		return this->ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void **>(o->ReleaseAndGetAddressOf()));
	}

	/**
	 * Casts the pointed interface to another one.
	 *
	 * @param[in] riid - referred interface ID
	 * @param[in,out] o - object to receive the casted interface pointer
	 * @return same as `QueryInterface`
	 */
	inline HRESULT AsIID(REFIID riid, ComPtr<IUnknown> * o) const noexcept {
		return this->ptr_->QueryInterface(riid, reinterpret_cast<void **>(o->ReleaseAndGetAddressOf()));
	}
protected:
	/**
	 * Increments the reference count of the interface associated with this `ComPtr`.
	 */
	inline void InternalAddRef() const noexcept {
		if (this->ptr_ != nullptr) this->ptr_->AddRef();
	}

	/**
	 * Performs a COM release operation on the interface associated with this `ComPtr`.
	 *
	 * @return new reference count
	 */
	inline ULONG InternalRelease() noexcept {
		if (this->ptr_ == nullptr) return 0;
		const ULONG res = this->ptr_->Release();
		this->ptr_ = nullptr;
		return res;
	}
};


} /* namespace WRL */
} /* namespace Microsoft */


template <typename T>
inline void ** IID_PPV_ARGS_Helper(::Microsoft::WRL::Details::ComPtrRef<T> pp) noexcept {
    static_assert(is_base_of<IUnknown, typename T::InterfaceType>::value, "expected COM interface");
    return pp;
}


#endif /* __PCF_UTILITYWINDOWS_HPP__ */
