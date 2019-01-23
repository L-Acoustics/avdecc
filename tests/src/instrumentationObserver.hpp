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
* @file instrumentationObserver.hpp
* @author Christophe Calmejane
*/

#pragma once

// Public API
#include <la/avdecc/internals/instrumentationNotifier.hpp>

#include <string>
#include <functional>
#include <list>
#include <mutex>

class InstrumentationObserver : public la::avdecc::InstrumentationNotifier::Observer
{
public:
	struct Action
	{
		std::string eventName{};
		std::function<void()> action{};
	};
	using Actions = std::list<Action>;

	InstrumentationObserver(Actions&& actions)
		: _actions(actions)
	{
	}

private:
	virtual void onEvent(std::string const& eventName) noexcept override
	{
		Action action;
		{
			std::lock_guard<decltype(_lock)> const lg(_lock);
			if (!_actions.empty())
			{
				if (_actions.front().eventName == eventName)
				{
					action = std::move(_actions.front());
					_actions.pop_front();
				}
			}
		}
		if (action.action)
		{
			action.action();
		}
	}

	std::mutex _lock{};
	Actions _actions{};
	DECLARE_AVDECC_OBSERVER_GUARD(InstrumentationObserver);
};
