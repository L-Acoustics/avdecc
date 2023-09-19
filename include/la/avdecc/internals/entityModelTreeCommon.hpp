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
* @file entityModelTreeCommon.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model tree common definitions.
*/

#pragma once

#include "entityModel.hpp"

#include <set>
#include <array>
#include <unordered_map>
#include <vector>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
struct StreamInputConnectionInfo
{
	enum class State
	{
		NotConnected = 0,
		FastConnecting = 1,
		Connected = 2,
	};
	StreamIdentification talkerStream{}; /** Only valid if state != State::NotConnected */
	State state{ State::NotConnected };
};

constexpr bool operator==(StreamInputConnectionInfo const& lhs, StreamInputConnectionInfo const& rhs) noexcept
{
	if (lhs.state == rhs.state)
	{
		// Only compare talkerStream field if State is not NotConnected
		return (lhs.state == StreamInputConnectionInfo::State::NotConnected) || (lhs.talkerStream == rhs.talkerStream);
	}
	return false;
}

constexpr bool operator!=(StreamInputConnectionInfo const& lhs, StreamInputConnectionInfo const& rhs) noexcept
{
	return !(lhs == rhs);
}

/** A subset of StreamInfo */
struct StreamDynamicInfo
{
	bool isClassB{ false }; /** Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	bool hasSavedState{ false }; /** Connection has saved ACMP state. */
	bool doesSupportEncrypted{ false }; /** Indicates that the Stream supports streaming with encrypted PDUs. */
	bool arePdusEncrypted{ false }; /** Indicates that the Stream is using encrypted PDUs. */
	bool hasTalkerFailed{ false }; /** Indicates that the Listener has registered an SRP Talker Failed attribute for the Stream. */
	StreamInfoFlags _streamInfoFlags{}; /** LEGACY FIELD - Last received StreamInfoFlags */
	std::optional<UniqueIdentifier> streamID{ std::nullopt };
	std::optional<std::uint32_t> msrpAccumulatedLatency{ std::nullopt };
	std::optional<la::networkInterface::MacAddress> streamDestMac{ std::nullopt };
	std::optional<MsrpFailureCode> msrpFailureCode{ std::nullopt };
	std::optional<BridgeIdentifier> msrpFailureBridgeID{ std::nullopt };
	std::optional<std::uint16_t> streamVlanID{ std::nullopt };
	// Milan additions
	std::optional<StreamInfoFlagsEx> streamInfoFlagsEx{ std::nullopt };
	std::optional<ProbingStatus> probingStatus{ std::nullopt };
	std::optional<protocol::AcmpStatus> acmpStatus{ std::nullopt };
};

constexpr bool operator==(StreamDynamicInfo const& lhs, StreamDynamicInfo const& rhs) noexcept
{
	return (lhs.isClassB == rhs.isClassB) && (lhs.hasSavedState == rhs.hasSavedState) && (lhs.doesSupportEncrypted == rhs.doesSupportEncrypted) && (lhs.arePdusEncrypted == rhs.arePdusEncrypted) && (lhs.hasTalkerFailed == rhs.hasTalkerFailed) && (lhs._streamInfoFlags == rhs._streamInfoFlags) && (lhs.streamID == rhs.streamID) && (lhs.msrpAccumulatedLatency == rhs.msrpAccumulatedLatency) && (lhs.streamDestMac == rhs.streamDestMac) && (lhs.msrpFailureCode == rhs.msrpFailureCode) && (lhs.msrpFailureBridgeID == rhs.msrpFailureBridgeID) && (lhs.streamVlanID == rhs.streamVlanID) && (lhs.streamInfoFlagsEx == rhs.streamInfoFlagsEx) && (lhs.probingStatus == rhs.probingStatus) && (lhs.acmpStatus == rhs.acmpStatus);
}

constexpr bool operator!=(StreamDynamicInfo const& lhs, StreamDynamicInfo const& rhs) noexcept
{
	return !(lhs == rhs);
}

/** A subset of AvbInfo */
struct AvbInterfaceInfo
{
	std::uint32_t propagationDelay{ 0u };
	AvbInfoFlags flags{};
	entity::model::MsrpMappings mappings{};
};

constexpr bool operator==(AvbInterfaceInfo const& lhs, AvbInterfaceInfo const& rhs) noexcept
{
	return (lhs.propagationDelay == rhs.propagationDelay) && (lhs.flags == rhs.flags) && (lhs.mappings == rhs.mappings);
}

constexpr bool operator!=(AvbInterfaceInfo const& lhs, AvbInterfaceInfo const& rhs) noexcept
{
	return !(lhs == rhs);
}

using StreamConnections = std::set<StreamIdentification>;
using StreamFormats = std::set<StreamFormat>;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
using RedundantStreams = std::set<StreamIndex>;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
using SamplingRates = std::set<SamplingRate>;
using AvdeccFixedStrings = std::array<AvdeccFixedString, 7>;
using ClockSources = std::vector<ClockSourceIndex>;
using PtpInstances = std::vector<PtpInstanceIndex>;
using DescriptorCounts = std::unordered_map<DescriptorType, std::uint16_t, utils::EnumClassHash>;

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
