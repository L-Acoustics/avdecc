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
* @file watchDog.hpp
* @author Christophe Calmejane
* @brief WatchDog helper.
*/

#pragma once

#include "exports.hpp"
#include <string>
#include <chrono>

namespace la
{
namespace avdecc
{
namespace watchDog
{
class WatchDog
{
public:
	static LA_AVDECC_API WatchDog& LA_AVDECC_CALL_CONVENTION getInstance() noexcept;

	virtual void registerWatch(std::string const& name, std::chrono::milliseconds const maximumInterval) noexcept = 0;
	virtual void unregisterWatch(std::string const& name) noexcept = 0;
	virtual void alive(std::string const& name) noexcept = 0;

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
