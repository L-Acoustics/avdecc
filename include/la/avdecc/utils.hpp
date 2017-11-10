/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file utils.hpp
* @author Christophe Calmejane
* @brief Useful templates and global methods.
*/

#pragma once

#include <type_traits>
#include <functional>
#include <iomanip> // setfill
#include <ios> // uppercase
#include <string> // string
#include <sstream> // stringstream
#include <limits> // numeric_limits
#include <set>
#include <mutex>
#include "internals/exports.hpp"

namespace la
{
namespace avdecc
{

LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION setCurrentThreadName(std::string const& name);

/** Useful template to be used with streams, it prevents a char (or uint8_t) to be printed as a char instead of the numeric value */
template <typename T>
constexpr auto forceNumeric(T const t) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer, "forceNumeric requires an integer value");

	// Promote a built-in type to at least (unsigned)int
	return +t;
}

/** Useful template to convert any integer value to it's hex representation. Can be filled with zeros (ex: int16(0x123) = 0x0123) and printed in uppercase. */
template<typename T>
std::string toHexString(T const v, bool const zeroFilled = false, bool const upper = false) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer, "toHexString requires an integer value");

	try
	{
		std::stringstream stream;
		stream << "0x";
		if (zeroFilled)
			stream << std::setfill('0') << std::setw(sizeof(T) * 2);
		if (upper)
			stream << std::uppercase;
		stream << std::hex << forceNumeric(v);
		return stream.str();
	}
	catch (...)
	{
		return "[Invalid Conversion]";
	}
}

/**
* @brief Returns the value of an enum as its underlying type.
* @param[in] e The enum to get the value of.
* @return The value of the enum as its underlying type.
*/
template<typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
constexpr auto to_integral(EnumType e) noexcept
{
	return static_cast<std::underlying_type_t<EnumType>>(e);
}

/**
* @brief Hash function for hash-type std-containers.
* @details Hash function for hash-type std-containers when using an enum as key.
*/
struct EnumClassHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

/**
* @brief Traits to easily handle enums.
* @details Available traits for enum:
*  - is_bitfield: Enables operators and methods to manipulate an enum that represents a bitfield.
*/
template<typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
struct enum_traits {};

/**
* @brief operator& for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType, typename = std::enable_if_t<enum_traits<EnumType>::is_bitfield>>
constexpr EnumType operator&(EnumType const lhs, EnumType const rhs)
{
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) & static_cast<std::underlying_type_t<EnumType>>(rhs));
}

/**
* @brief operator| for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType, typename = std::enable_if_t<enum_traits<EnumType>::is_bitfield>>
constexpr EnumType operator|(EnumType const lhs, EnumType const rhs)
{
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) | static_cast<std::underlying_type_t<EnumType>>(rhs));
}

/**
* @brief operator|= for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType, typename = std::enable_if_t<enum_traits<EnumType>::is_bitfield>>
constexpr EnumType& operator|=(EnumType& lhs, EnumType const rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

/**
* @brief Test method for a bitfield enum to check if the specified flag is set.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to be tested for the presence of the specified flag.
* @param[in] flag Flag to test the presence of in specified value.
* @return Returns true if the specified flag is set in the specified value.
*/
template<typename EnumType>
constexpr typename std::enable_if_t<enum_traits<EnumType>::is_bitfield, bool> hasFlag(EnumType const value, EnumType const flag)
{
	return (value & flag) != static_cast<EnumType>(0);
}

/**
* @brief Test method for a bitfield enum to check if any flag is set.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to be tested for the presence of any flag.
* @return Returns true if any flag is set in the specified value.
*/
template<typename EnumType>
constexpr typename std::enable_if_t<enum_traits<EnumType>::is_bitfield, bool> hasAnyFlag(EnumType const value)
{
	return value != static_cast<EnumType>(0);
}


/**
* @brief Function to safely call a handler (in the form of a std::function), forwarding all parameters to it.
* @details Calls the specified handler, protecting the caller from any thrown exception in the handler itself.
*/
template<typename T, typename... Ts>
void invokeProtectedHandler(std::function<T> const& handler, Ts&&... params) noexcept
{
	if (handler)
	{
		try
		{
			handler(std::forward<Ts>(params)...);
		}
		catch (...)
		{
		}
	}
}

/**
* @brief Function to safely call a class method, forwarding all parameters to it.
* @details Calls the specified class method, protecting the caller from any thrown exception in the handler itself.
*/
template<typename Method, class Object, typename... Parameters>
void invokeProtectedMethod(Method&& method, Object* const object, Parameters&&... params) noexcept
{
	if (method != nullptr && object != nullptr)
	{
		try
		{
			(object->*method)(std::forward<Parameters>(params)...);
		}
		catch (...)
		{
		}
	}
}

/** Useful template to create strongly typed defines that can be extended using inheritance */
template<typename DataType, typename = std::enable_if_t<std::is_arithmetic<DataType>::value || std::is_enum<DataType>::value>>
class TypedDefine
{
public:
	using value_type = DataType;

	explicit TypedDefine(value_type const value) noexcept
		: _value(value)
	{
	}

	value_type getValue() const noexcept
	{
		return _value;
	}

	void setValue(value_type const value) noexcept
	{
		_value = value;
	}

	explicit operator value_type() const noexcept
	{
		return _value;
	}

	bool operator==(TypedDefine const& v) const noexcept
	{
		return v.getValue() == _value;
	}

	bool operator!=(TypedDefine const& v) const noexcept
	{
		return !operator==(v);
	}

	struct Hash
	{
		std::size_t operator()(TypedDefine t) const
		{
			return static_cast<std::size_t>(t._value);
		}
	};

	// Defaulted compiler auto-generated methods
	TypedDefine(TypedDefine&&) = default;
	TypedDefine(TypedDefine const&) = default;
	TypedDefine& operator=(TypedDefine const&) = default;
	TypedDefine& operator=(TypedDefine&&) = default;

private:
	value_type _value{};
};

/**
* @brief Traits to easily handle TypedDefine.
* @details Available traits for TypedDefine:
*  - is_bitfield: Enables operators and methods to manipulate a TypedDefine that represents a bitfield.
*/
template<typename TypedDefineType, typename = std::enable_if_t<std::is_base_of<TypedDefine<typename TypedDefineType::value_type>, TypedDefineType>::value>>
struct typed_define_traits {};

/**
* @brief operator& for a bitfield TypedDefine.
* @details The is_bitfield trait must be defined to true.
*/
template<typename TypedDefineType, typename = std::enable_if_t<typed_define_traits<TypedDefineType>::is_bitfield>>
constexpr TypedDefineType operator&(TypedDefineType const& lhs, TypedDefineType const& rhs)
{
	return TypedDefineType(lhs.getValue() & rhs.getValue());
}

/**
* @brief operator| for a bitfield TypedDefine.
* @details The is_bitfield trait must be defined to true.
*/
template<typename TypedDefineType, typename = std::enable_if_t<typed_define_traits<TypedDefineType>::is_bitfield>>
constexpr TypedDefineType operator|(TypedDefineType const& lhs, TypedDefineType const& rhs)
{
	return TypedDefineType(lhs.getValue() | rhs.getValue());
}

/** EmptyLock implementing the BasicLockable concept. Can be used as template parameter for Observer and Subject classes in this file. */
class EmptyLock
{
public:
	void lock() noexcept
	{
	}
	void unlock() noexcept
	{
	}
	// Defaulted compiler auto-generated methods
	EmptyLock() = default;
	~EmptyLock() = default;
	EmptyLock(EmptyLock&&) = default;
	EmptyLock(EmptyLock const&) = default;
	EmptyLock& operator=(EmptyLock const&) = default;
	EmptyLock& operator=(EmptyLock&&) = default;
};

// Forward declare Subject template class
template<class Derived, class Mut> class Subject;

/** Dummy struct required to postpone the Observer template resolution when it's actually needed. This is required because of the forward declaration of the Subject template class, in order to access it's mutex_type typedef. */
template<typename Subject>
struct GetMutexType
{
	using type = typename Subject::mutex_type;
};


/**
* @brief An observer base class.
* @details Observer base class using RAII concept to automatically remove itself from Subjects upon destruction.
* @note This class is thread-safe and the mutex type can be specified as template parameter.<BR>
*       A no lock version is possible using EmptyLock instead of a real mutex type.
*/
template<class Observable>
class Observer
{
public:
	using mutex_type = typename GetMutexType<Observable>::type;
	virtual ~Observer()
	{
		// Unregister from all the Subjects
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		for (auto subject : _subjects)
		{
			try
			{
				subject->removeObserver(this);
			}
			catch (std::invalid_argument const&)
			{
				// Ignore error
			}
		}
	}

	/**
	* @brief Gets the count of currently registered Subjects.
	* @return The count of currently registered Subjects.
	*/
	size_t countSubjects() const
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		return _subjects.size();
	}

private:
	friend class Subject<Observable, mutex_type>;

	void registerSubject(Observable* const subject)
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer already registered
		if (_subjects.find(subject) != _subjects.end())
			throw std::invalid_argument("Subject already registered");
		// Add Subject
		_subjects.insert(subject);
	}

	void unregisterSubject(Observable* const subject)
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if Subject is registered
		auto const it = _subjects.find(subject);
		if (it == _subjects.end())
			throw std::invalid_argument("Subject not registered");
		// Remove Subject
		_subjects.erase(it);
	}

	// Private variables
	mutable mutex_type _mutex{};
	std::set<Observable*> _subjects{};
};


/**
* @brief A Subject base class.
* @details Subject base class using RAII concept to automatically remove itself from observers upon destruction.<BR>
*          The convenience method #notifyObservers or #notifyObserversMethod shall be used to notify all observers in a thread-safe way.
* @note This class is thread-safe and the mutex type can be specified as template parameter.<BR>
*       A no lock version is possible using EmptyLock instead of a real mutex type.<BR>
*       Subject and Observer classes use CRTP pattern.
* @warning If the derived class wants to allow observers to be destroyed inside an event handler, it shall use a
*          recursive mutex kind as template parameter.
*/
template<class Derived, class Mut>
class Subject
{
public:
	using mutex_type = Mut;
	using observer_type = Observer<Derived>;

	void registerObserver(observer_type* const observer)
	{
		if (observer == nullptr)
			throw std::invalid_argument("Observer cannot be nullptr");

		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer already registered
		if (_observers.find(observer) != _observers.end())
			throw std::invalid_argument("Observer already registered");
		// Register to the observer
		try
		{
			observer->registerSubject(self());
		}
		catch (std::invalid_argument const&)
		{
			// Already registered
		}
		// Add observer
		_observers.insert(observer);
	}

	/**
	* @brief Unregisters an observer.
	* @details Unregisters an observer from the subject.
	* @param[in] observer The observer to remove from the list.
	* @note If the observer has never been registered, or has already been
	*       unregistered, this method will throw an std::invalid_argument.
	*       If you try to unregister an observer during a notification without
	*       using a recursive mutex kind, this method will throw a std::system_error
	*       and the observer will not be removed (strong exception guarantee).
	*/
	void unregisterObserver(observer_type* const observer)
	{
		if (observer == nullptr)
			throw std::invalid_argument("Observer cannot be nullptr");

		// Unregister from the observer
		try
		{
			observer->unregisterSubject(self());
		}
		catch (std::invalid_argument const&)
		{
			// Already unregistered, but we continue so the removeObserver method will throw to the user
		}
		catch (...) // Catch mutex errors (or any other)
		{
			// Rethrow the last exception without trying to remove the observer
			throw;
		}
		// Remove observer
		try
		{
			removeObserver(observer);
		}
		catch (std::invalid_argument const&)
		{
			// Already unregistered, rethrow to the user
			throw;
		}
		catch (...) // Catch mutex errors (or any other)
		{
			// Restore the state by re-registering to the observer, then rethraw
			observer->registerSubject(self());
			throw;
		}
	}

	/** BasicLockable concept 'lock' method for the whole Subject */
	void lock()
	{
		_mutex.lock();
	}

	/** BasicLockable concept 'unlock' method for the whole Subject */
	void unlock()
	{
		_mutex.unlock();
	}

	/**
	* @brief Gets the count of currently registered observers.
	* @return The count of currently registered observers.
	*/
	size_t countObservers() const
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		return _observers.size();
	}

	/**
	* @brief Checks if specified observer is currently registered
	* @param[in] observer Observer to check for
	* @return Returns true if the specified observer is currently registered, false otherwise.
	*/
	bool isObserverRegistered(observer_type* const observer) const
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer is registered
		return _observers.find(observer) != _observers.end();
	}

	virtual ~Subject()
	{
		removeAllObservers();
	}

protected:
	/**
	* @brief Convenience method to notify all observers.
	* @details Convenience method to notify all observers in a thread-safe way.
	* @param[in] evt An std::function to be called for each registered observer.
	* @note The internal class lock will be taken during the whole call, meaning
	*       you cannot call another method of this class in the provided event handler
	*       except if the class template parameter is a recursive mutex kind (or EmptyLock).
	*/
	template<class DerivedObserver>
	void notifyObservers(std::function<void(DerivedObserver* obs)> const& evt) const noexcept
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		_iteratingNotify = true;
		// Call each observer
		for (auto it = _observers.begin(); it != _observers.end(); ++it)
		{
			// Do call an observer in the to be removed list
			if (_toBeRemoved.find(*it) != _toBeRemoved.end())
				continue;
			// Using try-catch to protect ourself from errors in the handler
			try
			{
				evt(static_cast<DerivedObserver*>(*it));
			}
			catch (...)
			{
			}
		}
		_iteratingNotify = false;
		// Time to remove observers scheduled for removal inside the notification loop
		for (auto* obs : _toBeRemoved)
		{
			try
			{
				removeObserver(obs);
			}
			catch (...)
			{
				// Don't care about already unregistered observer
			}
		}
		_toBeRemoved.clear();
	}

	/**
	* @brief Convenience method to notify all observers.
	* @details Convenience method to notify all observers in a thread-safe way.
	* @param[in] method The Observer method to be called. The first parameter of the method should always be self().
	* @param[in] params Variadic parameters to be forwarded to the method for each observer.
	* @note The internal class lock will be taken during the whole call, meaning
	*       you cannot call another method of this class in the provided event handler
	*       except if the class template parameter is a recursive mutex kind (or EmptyLock).
	*/
	template<class DerivedObserver, typename Method, typename... Parameters>
	void notifyObserversMethod(Method&& method, Parameters&&... params) const noexcept
	{
		if (method != nullptr)
		{
			// Lock observers
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			_iteratingNotify = true;
			// Call each observer
			for (auto it = _observers.begin(); it != _observers.end(); ++it)
			{
				// Do call an observer in the to be removed list
				if (_toBeRemoved.find(*it) != _toBeRemoved.end())
					continue;
				// Using try-catch to protect ourself from errors in the handler
				try
				{
					(static_cast<DerivedObserver*>(*it)->*method)(self(), std::forward<Parameters>(params)...);
				}
				catch (...)
				{
				}
			}
			_iteratingNotify = false;
			// Time to remove observers scheduled for removal inside the notification loop
			for (auto* obs : _toBeRemoved)
			{
				try
				{
					removeObserver(obs);
				}
				catch (...)
				{
					// Don't care about already unregistered observer
				}
			}
			_toBeRemoved.clear();
		}
	}

	/**
	* @brief Remove all observers from the subject.
	* @details Remove all observers from the subject.
	*/
	void removeAllObservers() noexcept
	{
		// Unregister from all the observers
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		for (auto const obs : _observers)
		{
			try
			{
				obs->unregisterSubject(self());
			}
			catch (std::invalid_argument const&)
			{
				// Ignore error
			}
		}
	}

	/** Convenience method to return this as the real Derived class type */
	Derived* self() noexcept
	{
		return static_cast<Derived*>(this);
	}

	/** Convenience method to return this as the real const Derived class type */
	Derived const* self() const noexcept
	{
		return static_cast<Derived const*>(this);
	}

private:
	friend class Observer<Derived>;

	void removeObserver(observer_type* const observer) const
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer is registered
		auto const it = _observers.find(observer);
		if (it == _observers.end())
			throw std::invalid_argument("Observer not registered");
		// Check if we are currently iterating notification
		if (_iteratingNotify)
			_toBeRemoved.insert(observer); // Schedule destruction for later
		else
			_observers.erase(it); // Remove observer immediately
	}

	// Private variables
	mutable Mut _mutex{};
	mutable std::set<observer_type*> _observers{};
	mutable bool _iteratingNotify{ false }; // Are we currently notifying observers
	mutable std::set<observer_type*> _toBeRemoved{};
};


/**
* @brief A Subject derived template class with tag dispatching.
* @details Subject derived template class publicly exposing notifyObservers and notifyObserversMethod.<BR>
*          Use this template along with a using directive to declare a tag dispatched Subject, so it can be used
*          as a class member (with direct access to notify methods).
*          Ex: using MySubject = TypedSubject<struct MySubjectTag, std::mutex>;
*/
template<typename Tag, class Mut>
class TypedSubject : public Subject<TypedSubject<Tag, Mut>, Mut>
{
public:
	using Subject<TypedSubject<Tag, Mut>, Mut>::notifyObservers;
	using Subject<TypedSubject<Tag, Mut>, Mut>::notifyObserversMethod;
	using Subject<TypedSubject<Tag, Mut>, Mut>::removeAllObservers;
};

} // namespace avdecc
} // namespace la
