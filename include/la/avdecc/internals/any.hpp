/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file any.hpp
* @author Christophe Calmejane
* @brief Temporary replacement for std::any, until it's available on all c++17 compilers.
* @note This implementation is not complete and do not support small object optimization.
*/

#pragma once

#if !defined(ENABLE_AVDECC_CUSTOM_ANY)
#	error "Including this file requires ENABLE_AVDECC_CUSTOM_ANY flag to be defined."
#endif // ENABLE_AVDECC_CUSTOM_ANY

// Include guard in case this exact same file is used (a copy of it) in another project ('pragma once' won't protect against this case)
#ifndef __ANY_FOR_CPP14_H_
#	define __ANY_FOR_CPP14_H_

#	include <type_traits>
#	include <utility>
#	include <typeinfo>

namespace std
{
namespace details
{
struct storage
{
	void* ptr{ nullptr };
};

template<typename T>
struct is_copy_constructible : ::std::is_constructible<T, typename ::std::add_lvalue_reference<typename ::std::add_const<T>::type>::type>::type
{
};

struct vtable
{
	void (*destroy)(storage&) noexcept;
	void (*copy)(storage const& src, storage& dst);
	void (*move)(storage& src, storage& dst) noexcept;
	void (*swap)(storage& lhs, storage& rhs) noexcept;
	::std::type_info const& (*type)() noexcept;
};

template<typename DecayedValueType>
struct dynamic_vtable
{
	static void destroy(storage& storage) noexcept
	{
		delete reinterpret_cast<DecayedValueType*>(storage.ptr);
		storage.ptr = nullptr;
	}
	static void copy(storage const& src, storage& dst)
	{
		dst.ptr = new DecayedValueType(*reinterpret_cast<DecayedValueType const*>(src.ptr));
	}
	static void move(storage& src, storage& dst) noexcept
	{
		dst.ptr = src.ptr;
		src.ptr = nullptr;
	}
	static void swap(storage& lhs, storage& rhs) noexcept
	{
		::std::swap(lhs.ptr, rhs.ptr);
	}
	static ::std::type_info const& type() noexcept
	{
		return typeid(DecayedValueType);
	}
};

template<typename DecayedValueType>
static vtable* get_vtable()
{
	using VTableType = dynamic_vtable<DecayedValueType>;
	static vtable s_table = {
		VTableType::destroy,
		VTableType::copy,
		VTableType::move,
		VTableType::swap,
		VTableType::type,
	};
	return &s_table;
}

} // namespace details

class bad_any_cast : public ::std::bad_cast
{
public:
	virtual char const* what() const noexcept override
	{
		return ("Bad any_cast");
	}
};

class any final
{
public:
	any() noexcept = default;

	any(any const& value)
		: _vtable(value._vtable)
	{
		if (value.has_value())
		{
			value._vtable->copy(value._storage, _storage);
		}
	}

	any(any&& value) noexcept
		: _vtable(value._vtable)
	{
		if (value.has_value())
		{
			value._vtable->move(value._storage, _storage);
			value._vtable = nullptr;
		}
	}

	template<typename ValueType, typename DecayedValueType = typename ::std::decay<ValueType>::type, typename = ::std::enable_if_t<!::std::is_same<DecayedValueType, any>::value>>
	constexpr any(ValueType&& value)
	{
		static_assert(details::is_copy_constructible<DecayedValueType>::value, "ValueType not CopyConstructible");
		_vtable = details::get_vtable<DecayedValueType>();
		_storage.ptr = new DecayedValueType(::std::forward<ValueType>(value));
	}

	~any()
	{
		reset();
	}

	any& operator=(any const& value)
	{
		any(value).swap(*this);
		return *this;
	}

	any& operator=(any&& value) noexcept
	{
		any(::std::move(value)).swap(*this);
		return *this;
	}

	template<typename ValueType, typename DecayedValueType = typename ::std::decay<ValueType>::type, typename = ::std::enable_if_t<!::std::is_same<DecayedValueType, any>::value>>
	constexpr any& operator=(ValueType&& value)
	{
		static_assert(details::is_copy_constructible<DecayedValueType>::value, "ValueType not CopyConstructible");
		any(::std::forward<ValueType>(value)).swap(*this);
		return *this;
	}

	void swap(any& value) noexcept
	{
		if (_vtable != value._vtable)
		{
			any tmp(::std::move(value));

			value._vtable = _vtable;
			if (_vtable != nullptr)
			{
				_vtable->move(_storage, value._storage);
			}

			_vtable = tmp._vtable;
			if (tmp._vtable != nullptr)
			{
				tmp._vtable->move(tmp._storage, _storage);
				tmp._vtable = nullptr;
			}
		}
		else
		{
			if (_vtable != nullptr)
				_vtable->swap(_storage, value._storage);
		}
	}

	void reset() noexcept
	{
		if (has_value())
		{
			_vtable->destroy(_storage);
			_vtable = nullptr;
		}
	}

	bool has_value() const noexcept
	{
		return _storage.ptr != nullptr;
	}

	::std::type_info const& type() const noexcept
	{
		return has_value() ? _vtable->type() : typeid(void);
	}

protected:
	template<typename ValueType>
	friend constexpr ValueType const* any_cast(any const* value) noexcept;
	template<typename ValueType>
	friend constexpr ValueType* any_cast(any* value) noexcept;

	template<typename ValueType /*, typename DecayedValueType = typename ::std::decay<ValueType>::type*/>
	ValueType const* reinterpret_value() const noexcept
	{
		return reinterpret_cast<ValueType const*>(_storage.ptr);
	}

	template<typename ValueType /*, typename DecayedValueType = typename ::std::decay<ValueType>::type*/>
	ValueType* reinterpret_value() noexcept
	{
		return reinterpret_cast<ValueType*>(_storage.ptr);
	}

private:
	details::vtable* _vtable{ nullptr };
	details::storage _storage{};
};


template<typename ValueType>
constexpr ValueType any_cast(any const& anyValue)
{
	static_assert(::std::is_constructible<ValueType, ::std::remove_cv_t<::std::remove_reference_t<ValueType>> const&>::value, "ValueType not constructible");
	auto const* const ptr = any_cast<::std::remove_cv_t<::std::remove_reference_t<ValueType>>>(&anyValue);
	if (ptr == nullptr)
	{
		throw bad_any_cast();
	}

	return static_cast<ValueType>(*ptr);
}

template<typename ValueType>
constexpr ValueType any_cast(any& anyValue)
{
	static_assert(::std::is_constructible<ValueType, ::std::remove_cv_t<::std::remove_reference_t<ValueType>>&>::value, "ValueType not constructible");
	auto const ptr = any_cast<::std::remove_reference_t<ValueType>>(&anyValue);
	if (ptr == nullptr)
	{
		throw bad_any_cast();
	}

	return static_cast<ValueType>(*ptr);
}

template<typename ValueType>
constexpr ValueType any_cast(any&& anyValue)
{
	static_assert(::std::is_constructible<ValueType, ::std::remove_cv_t<::std::remove_reference_t<ValueType>>>::value, "ValueType not constructible");
	auto const ptr = any_cast<::std::remove_cv_t<::std::remove_reference_t<ValueType>>>(&anyValue);
	if (ptr != nullptr)
	{
		throw bad_any_cast();
	}

	return static_cast<ValueType>(::std::move(*ptr));
}

template<typename ValueType>
constexpr ValueType const* any_cast(any const* value) noexcept
{
	if (value == nullptr || value->type() != typeid(ValueType))
		return nullptr;
	else
		return value->reinterpret_value<ValueType>();
}

template<typename ValueType>
constexpr ValueType* any_cast(any* value) noexcept
{
	if (value == nullptr || value->type() != typeid(ValueType))
		return nullptr;
	else
		return value->reinterpret_value<ValueType>();
}

template<typename ValueType>
inline any make_any(ValueType&& value)
{
	return any(std::move(value));
}

inline void swap(any& lhs, any& rhs) noexcept
{
	lhs.swap(rhs);
}

} // namespace std

#endif //!__ANY_FOR_CPP14_H_
