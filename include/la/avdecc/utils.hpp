/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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
#include <cstdarg>
#include <iomanip> // setprecision / setfill
#include <ios> // uppercase
#include <string> // string
#include <sstream> // stringstream
#include <limits> // numeric_limits
#include <set>
#include <mutex>
#include "internals/exports.hpp"
#include "internals/uniqueIdentifier.hpp"

namespace la
{
namespace avdecc
{

LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION setCurrentThreadName(std::string const& name);
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION enableAssert() noexcept;
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION disableAssert() noexcept;
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isAssertEnabled() noexcept;
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION displayAssertDialog(char const* const file, unsigned const line, char const* const message, va_list arg) noexcept;

template<typename Cond>
bool avdeccAssert(char const* const file, unsigned const line, Cond const condition, char const* const message, ...) noexcept
{
	bool const result = !!condition;
	if (isAssertEnabled())
	{
		if (!result)
		{
			va_list argptr;
			va_start(argptr, message);
			displayAssertDialog(file, line, message, argptr);
			va_end(argptr);
		}
	}
	return result;
}

template<typename Cond>
bool avdeccAssert(char const* const file, unsigned const line, Cond const condition, std::string const& message) noexcept
{
	return avdeccAssert(file, line, condition, message.c_str());
}

template<typename Cond>
bool avdeccAssertRelease(Cond const condition) noexcept
{
	bool const result = !!condition;
	return result;
}

} // namespace avdecc
} // namespace la

#if defined(DEBUG) || defined(COMPILE_AVDECC_ASSERT)
#define AVDECC_ASSERT(cond, msg, ...) (void)la::avdecc::avdeccAssert(__FILE__, __LINE__, cond, msg, ##__VA_ARGS__)
#define AVDECC_ASSERT_WITH_RET(cond, msg, ...) la::avdecc::avdeccAssert(__FILE__, __LINE__, cond, msg, ##__VA_ARGS__)
#else // !DEBUG && !COMPILE_AVDECC_ASSERT
#define AVDECC_ASSERT(cond, msg, ...)
#define AVDECC_ASSERT_WITH_RET(cond, msg, ...) la::avdecc::avdeccAssertRelease(cond)
#endif // DEBUG || COMPILE_AVDECC_ASSERT

namespace la
{
namespace avdecc
{

/** Useful template to be used with streams, it prevents a char (or uint8_t) to be printed as a char instead of the numeric value */
template <typename T>
constexpr auto forceNumeric(T const t) noexcept
{
	static_assert(std::is_arithmetic<T>::value, "forceNumeric requires an arithmetic value");

	// Promote a built-in type to at least (unsigned)int
	return +t;
}

/** Useful template to convert any integer value to it's hex representation. Can be filled with zeros (ex: int16(0x123) = 0x0123) and printed in uppercase. */
template<typename T>
constexpr std::string toHexString(T const v, bool const zeroFilled = false, bool const upper = false) noexcept
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

/** UniqueIdentifier overload */
template<>
inline std::string toHexString<UniqueIdentifier>(UniqueIdentifier const v, bool const zeroFilled, bool const upper) noexcept
{
	return toHexString(v.getValue(), zeroFilled, upper);
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

} // namespace avdecc
} // namespace la

/* The following operator overloads are declared in the global namespace, so they can easily be accessed (as long as the traits are enabled for the desired enum) */

/**
* @brief operator& for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType> operator&(EnumType const lhs, EnumType const rhs)
{
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) & static_cast<std::underlying_type_t<EnumType>>(rhs));
}

/**
* @brief operator| for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType> operator|(EnumType const lhs, EnumType const rhs)
{
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) | static_cast<std::underlying_type_t<EnumType>>(rhs));
}

/**
* @brief operator|= for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType>& operator|=(EnumType& lhs, EnumType const rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

/**
* @brief operator&= for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType>& operator&=(EnumType& lhs, EnumType const rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

/**
* @brief operator~ for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType> operator~(EnumType e)
{
	return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(e));
}

namespace la
{
namespace avdecc
{

/**
* @brief Test method for a bitfield enum to check if the specified flag is set.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to be tested for the presence of the specified flag.
* @param[in] flag Flag to test the presence of in specified value.
* @return Returns true if the specified flag is set in the specified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, bool> hasFlag(EnumType const value, EnumType const flag)
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
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, bool> hasAnyFlag(EnumType const value)
{
	return value != static_cast<EnumType>(0);
}

/**
* @brief Adds a flag to a bitfield enum. Multiple flags can be added at once if combined using operator|.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to which the specified flag(s) is(are) to be added.
* @param[in] flag Flag(s) to be added to the specified value.
* @note Effectively equivalent to value |= flag
* @return Returns a copy of the modified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType> addFlag(EnumType& value, EnumType const flag)
{
	value |= flag;

	return value;
}

/**
* @brief Clears a flag from a bitfield enum. Multiple flags can be cleared at once if combined using operator|.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to which the specified flag(s) is(are) to be removed.
* @param[in] flag Flag(s) to be cleared from the specified value.
* @note Effectively equivalent to value &= ~flag
* @return Returns a copy of the modified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::enum_traits<EnumType>::is_bitfield, EnumType> clearFlag(EnumType& value, EnumType const flag)
{
	value &= ~flag;

	return value;
}

/**
* @brief Function to safely call a handler (in the form of a std::function), forwarding all parameters to it.
* @param[in] handler The callable object to be invoked.
* @param[in] params The parameters to pass to the handler.
* @details Calls the specified handler in the current thread, protecting the caller from any thrown exception in the handler itself.
*/
template<typename CallableType, typename... Ts>
void invokeProtectedHandler(CallableType&& handler, Ts&&... params) noexcept
{
	if (handler)
	{
		try
		{
			handler(std::forward<Ts>(params)...);
		}
		catch (std::exception const& e)
		{
			/* Forcing the assert to fail, but with code so we don't get a warning on gcc */
			AVDECC_ASSERT(handler == nullptr, (std::string("invokeProtectedHandler caught an exception in handler: ") + e.what()).c_str());
			(void)e;
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
		catch (std::exception const& e)
		{
			/* Forcing the assert to fail, but with code so we don't get a warning on gcc */
			AVDECC_ASSERT(method == nullptr, (std::string("invokeProtectedMethod caught an exception in method: ") + e.what()).c_str());
			(void)e;
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

} // namespace avdecc
} // namespace la

/* The following operator overloads are declared in the global namespace, so they can easily be accessed (as long as the traits are enabled for the desired enum) */

/**
* @brief operator& for a bitfield TypedDefine.
* @details The is_bitfield trait must be defined to true.
*/
template<typename TypedDefineType>
constexpr std::enable_if_t<la::avdecc::typed_define_traits<TypedDefineType>::is_bitfield, TypedDefineType> operator&(TypedDefineType const& lhs, TypedDefineType const& rhs)
{
	return TypedDefineType(lhs.getValue() & rhs.getValue());
}

/**
* @brief operator| for a bitfield TypedDefine.
* @details The is_bitfield trait must be defined to true.
*/
template<typename TypedDefineType>
constexpr std::enable_if_t<la::avdecc::typed_define_traits<TypedDefineType>::is_bitfield, TypedDefineType> operator|(TypedDefineType const& lhs, TypedDefineType const& rhs)
{
	return TypedDefineType(lhs.getValue() | rhs.getValue());
}

namespace la
{
namespace avdecc
{

/** EmptyLock implementing the BasicLockable concept. Can be used as template parameter for Observer and Subject classes in this file. */
class EmptyLock
{
public:
	void lock() const noexcept
	{
	}
	void unlock() const noexcept
	{
	}
	// Defaulted compiler auto-generated methods
	EmptyLock() noexcept = default;
	~EmptyLock() noexcept = default;
	EmptyLock(EmptyLock&&) noexcept = default;
	EmptyLock(EmptyLock const&) noexcept = default;
	EmptyLock& operator=(EmptyLock const&) noexcept = default;
	EmptyLock& operator=(EmptyLock&&) noexcept = default;
};

// Forward declare Subject template class
template<class Derived, class Mut> class Subject;
// Forward declare ObserverGuard template class
template<class ObserverType> class ObserverGuard;

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
* @warning Not catching std::system_error from mutex, which will cause std::terminate() to be called if a critical system error occurs
*/
template<class Observable>
class Observer
{
public:
	using mutex_type = typename GetMutexType<Observable>::type;

	virtual ~Observer() noexcept
	{
		// Lock Subjects
		try
		{
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			AVDECC_ASSERT(_subjects.empty(), "All subjects must be unregistered before Observer is destroyed. Either manually call subject->unregisterObserver or add an ObserverGuard member at the end of your Observer derivated class.");
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
		catch (...)
		{
		}
	}

	/**
	* @brief Gets the count of currently registered Subjects.
	* @return The count of currently registered Subjects.
	*/
	size_t countSubjects() const noexcept
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		return _subjects.size();
	}

	// Defaulted compiler auto-generated methods
	Observer() = default;
	Observer(Observer&&) = default;
	Observer(Observer const&) = default;
	Observer& operator=(Observer const&) = default;
	Observer& operator=(Observer&&) = default;

private:
	friend class Subject<Observable, mutex_type>;
	template<class ObserverType>
	friend class ObserverGuard;

	void removeAllSubjects() noexcept
	{
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
		_subjects.clear();
	}

	void registerSubject(Observable const* const subject) const
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer already registered
		if (_subjects.find(subject) != _subjects.end())
			throw std::invalid_argument("Subject already registered");
		// Add Subject
		_subjects.insert(subject);
	}

	void unregisterSubject(Observable const* const subject) const
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
	mutable std::set<Observable const*> _subjects{};
};

/**
* @brief An Observer guard class to allow safe RAII usage of an Observer class.
* @details This class is intended to be used in conjunction with an Observer class to allow safe RAII usage of the Observer.<BR>
*          Because a class has to inherit from an Observer to override notification handlers, it means by the time the Observer destructor is called
*          the derivated class portion has already been destroyed. If the subject is notifying observers at the same time from another thread, the derivated
*          vtable is no longer valid and will cause a crash. This class can be used for full RAII by simply declaring a member of this type in the derivated class,
*          at the end of the data members list.
* @warning Not catching std::system_error from mutex, which will cause std::terminate() to be called if a critical system error occurs
*/
template<class ObserverType>
class ObserverGuard final
{
public:
	ObserverGuard(ObserverType& observer) noexcept
		: _observer(observer)
	{
	}

	~ObserverGuard() noexcept
	{
		_observer.removeAllSubjects();
	}

	// Defaulted compiler auto-generated methods
	ObserverGuard(ObserverGuard&&) noexcept = default;
	ObserverGuard(ObserverGuard const&) noexcept = default;
	ObserverGuard& operator=(ObserverGuard const&) noexcept = default;
	ObserverGuard& operator=(ObserverGuard&&) noexcept = default;

private:
	ObserverType & _observer{ nullptr };
};

/**
* @brief A Subject base class.
* @details Subject base class using RAII concept to automatically remove itself from observers upon destruction.<BR>
*          The convenience method #notifyObservers or #notifyObserversMethod shall be used to notify all observers in a thread-safe way.
* @note This class is thread-safe and the mutex type can be specified as template parameter.<BR>
*       A no lock version is possible using EmptyLock instead of a real mutex type.<BR>
*       Subject and Observer classes use CRTP pattern.
* @warning If the derived class wants to allow observers to be destroyed inside an event handler, it shall use a
*          recursive mutex kind as template parameter.<BR>
*          Not catching std::system_error from mutex, which will cause std::terminate() to be called if a critical system error occurs
*/
template<class Derived, class Mut>
class Subject
{
public:
	using mutex_type = Mut;
	using observer_type = Observer<Derived>;

	void registerObserver(observer_type* const observer) const
	{
		if (observer == nullptr)
			throw std::invalid_argument("Observer cannot be nullptr");

		{
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

		// Inform the subject that a new observer has registered
		try
		{
			const_cast<Subject*>(this)->onObserverRegistered(observer);
		}
		catch (...)
		{
			// Ignore exceptions in handler
		}
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
	void unregisterObserver(observer_type* const observer) const
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
	void lock() noexcept
	{
		_mutex.lock();
	}

	/** BasicLockable concept 'unlock' method for the whole Subject */
	void unlock() noexcept
	{
		_mutex.unlock();
	}

	/**
	* @brief Gets the count of currently registered observers.
	* @return The count of currently registered observers.
	*/
	size_t countObservers() const noexcept
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
	bool isObserverRegistered(observer_type* const observer) const noexcept
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer is registered
		return _observers.find(observer) != _observers.end();
	}

	virtual ~Subject() noexcept
	{
		removeAllObservers();
	}

	// Defaulted compiler auto-generated methods
	Subject() = default;
	Subject(Subject&&) = default;
	Subject(Subject const&) = default;
	Subject& operator=(Subject const&) = default;
	Subject& operator=(Subject&&) = default;

protected:
#ifdef DEBUG
	mutex_type& getMutex() noexcept
	{
		return _mutex;
	}

	mutex_type const& getMutex() const noexcept
	{
		return _mutex;
	}
#endif // DEBUG

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
					(static_cast<DerivedObserver*>(*it)->*method)(std::forward<Parameters>(params)...);
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

	/** Remove all observers from the subject. */
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

	/** Allow the Subject to be informed when a new observer has registered. */
	virtual void onObserverRegistered(observer_type* const /*observer*/) noexcept
	{
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

#define DECLARE_AVDECC_OBSERVER_GUARD_NAME(selfClassType, variableName) \
	friend class la::avdecc::ObserverGuard<selfClassType>; \
	la::avdecc::ObserverGuard<selfClassType> variableName{ *this }

#define DECLARE_AVDECC_OBSERVER_GUARD(selfClassType) \
	friend class la::avdecc::ObserverGuard<selfClassType>; \
	la::avdecc::ObserverGuard<selfClassType> _observer_guard_{ *this }

} // namespace avdecc
} // namespace la
