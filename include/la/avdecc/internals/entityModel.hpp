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
* @file entityModel.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model.
*/

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <cassert>
#include <unordered_map>
#include <iostream>
#include <tuple>
#include "entityEnums.hpp"
#include "uniqueIdentifier.hpp"
#include "protocolDefines.hpp"
#include "entityModelTypes.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/networkInterfaceHelper.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{

constexpr VendorEntityModel getNullVendorEntityModel() noexcept
{
	return VendorEntityModel(0u);
}

constexpr StreamFormat getNullStreamFormat() noexcept
{
	return StreamFormat(0u);
}

constexpr SamplingRate getNullSamplingRate() noexcept
{
	return SamplingRate(0u);
}

enum class DescriptorType : std::uint16_t
{
	Entity = 0,
	Configuration = 1,
	AudioUnit = 2,
	VideoUnit = 3,
	SensorUnit = 4,
	StreamInput = 5,
	StreamOutput = 6,
	JackInput = 7,
	JackOutput = 8,
	AvbInterface = 9,
	ClockSource = 10,
	MemoryObject = 11,
	Locale = 12,
	Strings = 13,
	StreamPortInput = 14,
	StreamPortOutput = 15,
	ExternalPortInput = 16,
	ExternalPortOutput = 17,
	InternalPortInput = 18,
	InternalPortOutput = 19,
	AudioCluster = 20,
	VideoCluster = 21,
	SensorCluster = 22,
	AudioMap = 23,
	VideoMap = 24,
	SensorMap = 25,
	Control = 26,
	SignalSelector = 27,
	Mixer = 28,
	Matrix = 29,
	MatrixSignal = 30,
	SignalSplitter = 31,
	SignalCombiner = 32,
	SignalDemultiplexer = 33,
	SignalMultiplexer = 34,
	SignalTranscoder = 35,
	ClockDomain = 36,
	ControlBlock = 37,
};
constexpr bool operator==(DescriptorType const lhs, DescriptorType const rhs)
{
	return static_cast<std::underlying_type_t<DescriptorType>>(lhs) == static_cast<std::underlying_type_t<DescriptorType>>(rhs);
}

constexpr bool operator==(DescriptorType const lhs, std::underlying_type_t<DescriptorType> const rhs)
{
	return static_cast<std::underlying_type_t<DescriptorType>>(lhs) == rhs;
}

struct CommonDescriptor
{
	DescriptorType descriptorType;
	DescriptorIndex descriptorIndex{ 0u };
};

/** ENTITY Descriptor - Clause 7.2.1 */

struct EntityDescriptor
{
	CommonDescriptor common{ DescriptorType::Entity };
	UniqueIdentifier entityID{ getNullIdentifier() };
	VendorEntityModel vendorEntityModelID{ getNullVendorEntityModel() };
	EntityCapabilities entityCapabilities{ EntityCapabilities::None };
	std::uint16_t talkerStreamSources{ 0u };
	TalkerCapabilities talkerCapabilities{ TalkerCapabilities::None };
	std::uint16_t listenerStreamSinks{ 0u };
	ListenerCapabilities listenerCapabilities{ ListenerCapabilities::None };
	ControllerCapabilities controllerCapabilities{ ControllerCapabilities::None };
	std::uint32_t availableIndex{ 0u };
	UniqueIdentifier associationID{ getNullIdentifier() };
	AvdeccFixedString entityName{};
	std::uint16_t vendorNameString{ 0u };
	std::uint16_t modelNameString{ 0u };
	AvdeccFixedString firmwareVersion{};
	AvdeccFixedString groupName{};
	AvdeccFixedString serialNumber{};
	std::uint16_t configurationsCount{ 0u };
	std::uint16_t currentConfiguration{ 0u };
};

/** CONFIGURATION Descriptor - Clause 7.2.2 */
struct ConfigurationDescriptor
{
	CommonDescriptor common{ DescriptorType::Configuration };
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{ LocalizedStringReference(0u) };
	std::uint16_t descriptorCountsCount{ 0u };
	std::uint16_t descriptorCountsOffset{ 0u };
	std::unordered_map<DescriptorType, std::uint16_t, la::avdecc::EnumClassHash> descriptorCounts{};
};

/** LOCALE Descriptor - Clause 7.2.11 */

struct LocaleDescriptor
{
	CommonDescriptor common{ DescriptorType::Locale };
	AvdeccFixedString localeID{};
	std::uint16_t numberOfStringDescriptors{ 0u };
	StringsIndex baseStringDescriptorIndex{ StringsIndex(0u) };
};

/** STRINGS Descriptor - Clause 7.2.12 */

struct StringsDescriptor
{
	CommonDescriptor common{ DescriptorType::Strings };
	std::array<AvdeccFixedString, 7> strings{};
};

/** STREAM_INPUT and STREAM_OUTPUT Descriptor - Clause 7.2.6 */
struct StreamDescriptor
{
	CommonDescriptor common{};
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{ LocalizedStringReference(0u) };
	std::uint16_t clockDomainIndex{ 0u };
	StreamFlags streamFlags{ StreamFlags::None };
	StreamFormat currentFormat{ getNullStreamFormat() };
	std::uint16_t formatsOffset{ 0u };
	std::uint16_t numberOfFormats{ 0u };
	UniqueIdentifier backupTalkerEntityID_0{ getNullIdentifier() };
	std::uint16_t backupTalkerUniqueID_0{ 0u };
	UniqueIdentifier backupTalkerEntityID_1{ getNullIdentifier() };
	std::uint16_t backupTalkerUniqueID_1{ 0u };
	UniqueIdentifier backupTalkerEntityID_2{ getNullIdentifier() };
	std::uint16_t backupTalkerUniqueID_2{ 0u };
	UniqueIdentifier backedupTalkerEntityID{ getNullIdentifier() };
	std::uint16_t backedupTalkerUnique{ 0u };
	std::uint16_t avbInterfaceIndex{ 0u };
	std::uint32_t bufferLength{ 0u };
	std::vector<StreamFormat> formats{};

	// Constructors
	StreamDescriptor() noexcept {} /* = default; */ // Cannot '= default' due to clang4.0 bug
	StreamDescriptor(DescriptorType const type, DescriptorIndex const descriptorIndex = 0u) noexcept
	{
		common.descriptorType = type;
		common.descriptorIndex = descriptorIndex;
	}
};

struct StreamConnectedState
{
	UniqueIdentifier talkerID{ getNullIdentifier() };
	StreamIndex talkerIndex{ StreamIndex(0u) };
	bool isConnected{ false };
	bool isFastConnect{ false };
};

constexpr bool operator==(StreamConnectedState const& lhs, StreamConnectedState const& rhs) noexcept
{
	return (lhs.talkerID == rhs.talkerID) && (lhs.talkerIndex == rhs.talkerIndex) && (lhs.isConnected == rhs.isConnected) && (lhs.isFastConnect == rhs.isFastConnect);
}

constexpr bool operator!=(StreamConnectedState const& lhs, StreamConnectedState const& rhs) noexcept
{
	return !(lhs == rhs);
}

struct AudioMapping
{
	std::uint16_t streamIndex{ 0u };
	std::uint16_t streamChannel{ 0u };
	std::uint16_t clusterOffset{ 0u };
	std::uint16_t clusterChannel{ 0u };
};
using AudioMappings = std::vector<AudioMapping>;

/** GET_STREAM_INFO and SET_STREAM_INFO Dynamic Information - Clause 7.4.16.2 */
struct StreamInfo
{
	StreamInfoFlags streamInfoFlags{ StreamInfoFlags::None };
	StreamFormat streamFormat{ getNullStreamFormat() };
	UniqueIdentifier streamID{ getNullIdentifier() };
	std::uint32_t msrpAccumulatedLatency{ 0u };
	la::avdecc::networkInterface::MacAddress streamDestMac{};
	std::uint8_t msrpFailureCode{ 0u };
	UniqueIdentifier msrpFailureBridgeID{ getNullIdentifier() };
	std::uint16_t streamVlanID{ 0u };
};

constexpr bool operator==(StreamInfo const& lhs, StreamInfo const& rhs) noexcept
{
	return (lhs.streamInfoFlags  == rhs.streamInfoFlags) && (lhs.streamFormat == rhs.streamFormat) &&
		(lhs.streamID == rhs.streamID) && (lhs.msrpAccumulatedLatency == rhs.msrpAccumulatedLatency) &&
		(lhs.streamDestMac == rhs.streamDestMac) && (lhs.msrpFailureCode == rhs.msrpFailureCode) &&
		(lhs.msrpFailureBridgeID == rhs.msrpFailureBridgeID) && (lhs.streamVlanID == rhs.streamVlanID);
}

/**
* @brief Make a VendorEntityModel from vendorID, deviceID and modelID.
* @details Make a VendorEntityModel from vendorID, deviceID and modelID.
* @param[in] vendorID OUI-24 of the vendor (8 MSBs should be 0, ignored regardless).
* @param[in] deviceID ID of the device (vendor specific).
* @param[in] modelID ID of the model (vendor specific).
* @return Packed vendorEntityModel to be used in ADP messages and EntityDescriptor.
*/
constexpr VendorEntityModel makeVendorEntityModel(std::uint32_t const vendorID, std::uint8_t const deviceID, std::uint32_t const modelID) noexcept
{
	return (static_cast<UniqueIdentifier>(vendorID) << 40) + (static_cast<UniqueIdentifier>(deviceID) << 32) + static_cast<UniqueIdentifier>(modelID);
}

/**
* @brief Split a VendorEntityModel into vendorID, deviceID and modelID.
* @details Split a VendorEntityModel into vendorID, deviceID and modelID.
* @param[in] vendorEntityModelID The VendorEntityModel value.
* @return Tuple of vendorID (OUI-24), deviceID and modelID.
*/
constexpr std::tuple<std::uint32_t, std::uint8_t, std::uint32_t> splitVendorEntityModel(VendorEntityModel const vendorEntityModelID) noexcept
{
	//std::uint32_t vendorID = (entityDescriptor.vendorEntityModelID >> 40) & 0x00FFFFFF; // Mask the OUI-24 vendor value (24 most significant bits)
	return std::make_tuple(
		static_cast<std::uint32_t>((vendorEntityModelID >> 40) & 0x0000000000FFFFFF),
		static_cast<std::uint8_t>((vendorEntityModelID >> 32) & 0x00000000000000FF),
		static_cast<std::uint32_t>(vendorEntityModelID & 0x00000000FFFFFFFF)
	);
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
