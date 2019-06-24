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
* @file networkInterfaceHelper_common.hpp
* @author Christophe Calmejane
* @brief OS independent network interface types and methods.
*/

#pragma once

#include "la/avdecc/networkInterfaceHelper.hpp"

#include <unordered_map>
#include <string>

namespace la
{
namespace avdecc
{
namespace networkInterface
{
using Interfaces = std::unordered_map<std::string, Interface>;

// Methods to be implemented by eachOS-dependant implementation
/** Block until the first enumeration occured */
void waitForFirstEnumeration() noexcept;
/** When the first observer is registered */
void onFirstObserverRegistered() noexcept;
/** When the last observer is unregistered */
void onLastObserverUnregistered() noexcept;

// Notifications from OS-dependant implementation
/** When the list of interfaces changed */
void onNewInterfacesList(Interfaces&& interfaces) noexcept;
/** When the Enabled state of an interface changed */
void onEnabledStateChanged(std::string const& interfaceName, bool const isEnabled) noexcept;
/** When the Connected state of an interface changed */
void onConnectedStateChanged(std::string const& interfaceName, bool const isConnected) noexcept;

} // namespace networkInterface
} // namespace avdecc
} // namespace la
