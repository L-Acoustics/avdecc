/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file instrumentationNotifier.hpp
* @author Christophe Calmejane
* @brief Instrumentation helper class.
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include <mutex>
#include <string>

namespace la
{
namespace avdecc
{
class InstrumentationNotifier final : public la::avdecc::utils::Subject<InstrumentationNotifier, la::avdecc::utils::EmptyLock>
{
public:
	class Observer : public la::avdecc::utils::Observer<InstrumentationNotifier>
	{
	public:
		virtual void onEvent(std::string const& eventName) noexcept = 0;
	};

	static InstrumentationNotifier const& getInstance() noexcept
	{
		static InstrumentationNotifier s_instance{};
		return s_instance;
	}

	void triggerEvent(std::string const& eventName) const noexcept
	{
		notifyObserversMethod<InstrumentationNotifier::Observer>(&InstrumentationNotifier::Observer::onEvent, eventName);
	}
};

} // namespace avdecc
} // namespace la
