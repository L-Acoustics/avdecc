/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file avdeccControlledEntityCommonModel.hpp
* @author Christophe Calmejane
* @brief Avdecc common entity model for a #la::avdecc::controller::ControlledEntity.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include <set>
#include <array>
#include <unordered_map>
#include <vector>

namespace la
{
namespace avdecc
{
namespace controller
{
namespace model
{
struct StreamConnectionState
{
	enum class State
	{
		NotConnected = 0,
		FastConnecting = 1,
		Connected = 2,
	};
	entity::model::StreamIdentification listenerStream{}; /** Always valid */
	entity::model::StreamIdentification talkerStream{}; /** Only valid if state != State::NotConnected */
	State state{ State::NotConnected };
};

constexpr bool operator==(StreamConnectionState const& lhs, StreamConnectionState const& rhs) noexcept
{
	if (lhs.state == rhs.state && lhs.listenerStream == rhs.listenerStream)
	{
		// Only compare talkerStream field if State is not NotConnected
		return (lhs.state == StreamConnectionState::State::NotConnected) || (lhs.talkerStream == rhs.talkerStream);
	}
	return false;
}

constexpr bool operator!=(StreamConnectionState const& lhs, StreamConnectionState const& rhs) noexcept
{
	return !(lhs == rhs);
}

using StreamConnections = std::set<entity::model::StreamIdentification>;
using StreamFormats = std::set<entity::model::StreamFormat>;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
using RedundantStreams = std::set<entity::model::StreamIndex>;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
using SamplingRates = std::set<entity::model::SamplingRate>;
using AvdeccFixedStrings = std::array<entity::model::AvdeccFixedString, 7>;
using ClockSources = std::vector<entity::model::ClockSourceIndex>;
using DescriptorCounts = std::unordered_map<entity::model::DescriptorType, std::uint16_t, utils::EnumClassHash>;

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
