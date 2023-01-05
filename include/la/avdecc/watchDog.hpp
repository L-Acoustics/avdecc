/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file watchDog.hpp
* @author Christophe Calmejane
* @brief WatchDog helper.
*/

#pragma once

#include "utils.hpp"
#include "internals/exports.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <chrono>

namespace la
{
namespace avdecc
{
namespace watchDog
{
/**
* @details Class to detect (dead)locked threads, or operations that took longer than expected.
*          Used as debugging purpose.
*          The class is implemented as a shared_ptr singleton, so it does not get deleted immediately when the program ends
*          (any pending async operation that hold a reference on the WatchDog can still use it).
*/
class WatchDog
{
protected:
	using Subject = utils::TypedSubject<struct SubjectTag, std::mutex>;

public:
	/** Observer interface for the WatchDog */
	class Observer : public la::avdecc::utils::Observer<Subject>
	{
	public:
		virtual ~Observer() noexcept {}
		virtual void onIntervalExceeded(std::string const& /*name*/, std::chrono::milliseconds const /*maximumInterval*/) noexcept {}
	};

	using SharedPointer = std::shared_ptr<WatchDog>; /**< Alias for a shared pointer on the class */

	static LA_AVDECC_API SharedPointer LA_AVDECC_CALL_CONVENTION getInstance() noexcept;

	virtual void registerObserver(Observer* const observer) noexcept = 0;
	virtual void unregisterObserver(Observer* const observer) noexcept = 0;

	virtual void registerWatch(std::string const& name, std::chrono::milliseconds const maximumInterval, bool const isThreadSpecific) noexcept = 0;
	virtual void unregisterWatch(std::string const& name, bool const isThreadSpecific) noexcept = 0;
	virtual void alive(std::string const& name, bool const isThreadSpecific) noexcept = 0;

	// Deleted compiler auto-generated methods
	WatchDog(WatchDog&&) = delete;
	WatchDog(WatchDog const&) = delete;
	WatchDog& operator=(WatchDog const&) = delete;
	WatchDog& operator=(WatchDog&&) = delete;

protected:
	WatchDog() noexcept = default;
	virtual ~WatchDog() noexcept = default;
};

} // namespace watchDog
} // namespace avdecc
} // namespace la
