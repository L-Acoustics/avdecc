/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file entityModelTreeDynamic.hpp
* @author Christophe Calmejane
* @brief Dynamic part of the avdecc entity model tree.
* @note This is the part of the AEM that can be changed dynamically, or that might be different from an Entity to another one with the same EntityModelID
*/

#pragma once

#include "entityModelTreeCommon.hpp"

#include <unordered_map>
#include <cstdint>
#include <map>
#include <optional>
#include <typeindex>
#include <unordered_map>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
using EntityCounters = std::map<entity::EntityCounterValidFlag, DescriptorCounter>;
using AvbInterfaceCounters = std::map<entity::AvbInterfaceCounterValidFlag, DescriptorCounter>;
using ClockDomainCounters = std::map<entity::ClockDomainCounterValidFlag, DescriptorCounter>;
using StreamInputCounters = std::map<entity::StreamInputCounterValidFlag, DescriptorCounter>;

/** Class representing the StreamOutputCounters (which are not equivalent based on Milan/IEEE spec). */
class StreamOutputCounters final
{
public:
	enum class CounterType
	{
		Unknown = 0,
		Milan_12 = 1,
		IEEE17221_2021 = 2,
	};

	/** Default constructor */
	StreamOutputCounters() noexcept = default;

	/** Constructor from base validFlags and base counters (explicit CounterType) */
	StreamOutputCounters(CounterType const counterType, DescriptorCounterValidFlag const validFlags, DescriptorCounters const counters) noexcept
		: _validFlags{ validFlags }
		, _counters{ counters }
	{
		setCounterType(counterType);
	}

	/** Constructor from map association format (deduced CounterType) */
	template<typename CountersType, typename ValidFlagType = typename CountersType::key_type, typename ValidFlagsType = utils::EnumBitfield<ValidFlagType>>
	StreamOutputCounters(CountersType const& counters) noexcept
	{
		static_assert(std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlagsMilan12> || std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlags17221>, "Invalid type for StreamOutputCounterValidFlags");

		// Set the counters
		setCounters(counters);
	}

	/** Get the CounterType */
	CounterType getCounterType() const noexcept
	{
		return _counterType;
	}

	/** Set the CounterType */
	void setCounterType(CounterType const counterType) noexcept
	{
		_counterType = counterType;
		_counterTypeHash = counterTypeToHash(_counterType);
	}

	/** Get the ValidFlags corresponding to the specified StreamOutputCounterValidFlags type. */
	template<typename ValidFlagsType>
	ValidFlagsType getValidFlags() const
	{
		static_assert(std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlagsMilan12> || std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlags17221>, "Invalid type for StreamOutputCounterValidFlags");

		// Check if requested template parameter type matches the internal CounterType
		if (typeid(ValidFlagsType).hash_code() != _counterTypeHash)
		{
			throw std::invalid_argument("Trying to getValidFlags() of for a type different than current CounterType");
		}

		return _validFlags.get<ValidFlagsType>();
	}

	/** Get a copy of the counters corresponding to the specified StreamOutputCounterValidFlags type, in the map association format. Throws if the requested type is different from the current CounterType. */
	template<typename ValidFlagsType, typename CountersType = std::map<typename ValidFlagsType::value_type, DescriptorCounter>>
	CountersType getCounters() const
	{
		static_assert(std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlagsMilan12> || std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlags17221>, "Invalid type for StreamOutputCounterValidFlags");

		// Check if requested template parameter type matches the internal CounterType
		if (typeid(ValidFlagsType).hash_code() != _counterTypeHash)
		{
			throw std::invalid_argument("Trying to getCounters() of for a type different than current CounterType");
		}

		return convertCounters<ValidFlagsType>();
	}

	/** Get a copy of the counters corresponding to the specified StreamOutputCounterValidFlags type, in the map association format. */
	template<typename ValidFlagsType, typename CountersType = std::map<typename ValidFlagsType::value_type, DescriptorCounter>>
	CountersType convertCounters() const noexcept
	{
		auto convertedCounters = CountersType{};
		auto validFlags = ValidFlagsType{};
		validFlags.assign(_validFlags.value());

		// Create a map of the valid flags to the corresponding counter
		for (auto const counter : validFlags)
		{
			convertedCounters[counter] = _counters[validFlags.getPosition(counter)];
		}

		return convertedCounters;
	}

	/** Set the counters from the specified StreamOutputCounterValidFlags type. Invalid map key (not a bit value) silently ignored. */
	template<typename CountersType, typename ValidFlagType = typename CountersType::key_type, typename ValidFlagsType = utils::EnumBitfield<ValidFlagType>>
	void setCounters(CountersType const& counters) noexcept
	{
		static_assert(std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlagsMilan12> || std::is_same_v<ValidFlagsType, StreamOutputCounterValidFlags17221>, "Invalid type for StreamOutputCounterValidFlags");

		// Convert template type to CounterType
		auto const hash = typeid(ValidFlagsType).hash_code();
		auto const counterType = hashToCounterType(hash);
		setCounterType(counterType);

		// Reset counters
		_counters = DescriptorCounters{};

		// Set the new data
		auto validFlags = decltype(_validFlags)::value_type{};
		for (auto const& [validFlag, counter] : counters)
		{
			auto const baseFlag = utils::to_integral(validFlag);
			// Validate the flag
			if (countBits(baseFlag) == 1)
			{
				validFlags += baseFlag;
				_counters[getBitPosition(baseFlag)] = counter;
			}
		}

		// Set the valid flags
		_validFlags = StreamOutputCounterValidFlags{ validFlags };
	}

	/** Get the valid flags of the base type. */
	DescriptorCounterValidFlag getBaseValidFlags() const noexcept
	{
		return _validFlags.value();
	}

	/** Get the counters of the base type. */
	DescriptorCounters const& getBaseCounters() const noexcept
	{
		return _counters;
	}

	/** Get the counters of the base type. */
	DescriptorCounters& getBaseCounters() noexcept
	{
		return _counters;
	}

	/** Operator+=. If same CounterType append the provided counters. If different type fully replace with the provided ones. */
	StreamOutputCounters& operator+=(StreamOutputCounters const& other) noexcept
	{
		enum class BaseFlag : DescriptorCounterValidFlag
		{
		};
		using BaseFlags = utils::EnumBitfield<BaseFlag>;

		// Check if the CounterType is the same
		if (_counterType == other._counterType)
		{
			auto otherValidFlags = BaseFlags{};
			otherValidFlags.assign(other._validFlags.value());
			auto validFlags = _validFlags.value();

			// Append the counters
			for (auto const validFlag : otherValidFlags)
			{
				auto const baseFlag = utils::to_integral(validFlag);
				auto const bitPosition = getBitPosition(baseFlag);
				validFlags += baseFlag;
				_counters[bitPosition] = other._counters[bitPosition];
			}

			// Set the valid flags
			_validFlags = StreamOutputCounterValidFlags{ validFlags };
		}
		else
		{
			// Replace the counters
			setCounterType(other._counterType);
			_counters = other._counters;
			_validFlags = other._validFlags;
		}
		return *this;
	}

private:
	static constexpr size_t countBits(StreamOutputCounterValidFlags::value_type const value) noexcept
	{
		return (value == 0u) ? 0u : 1u + countBits(value & (value - 1u));
	}
	static constexpr size_t getBitPosition(StreamOutputCounterValidFlags::value_type const value, StreamOutputCounterValidFlags::value_type const setBitValue = static_cast<StreamOutputCounterValidFlags::value_type>(-1)) noexcept
	{
		return (value == 1u) ? 0u : (setBitValue & 0x1) + getBitPosition(value >> 1, setBitValue >> 1);
	}
	static inline size_t counterTypeToHash(CounterType const counterType)
	{
		switch (counterType)
		{
			case CounterType::Unknown:
				return 0;
			case CounterType::Milan_12:
				return typeid(StreamOutputCounterValidFlagsMilan12).hash_code();
			case CounterType::IEEE17221_2021:
				return typeid(StreamOutputCounterValidFlags17221).hash_code();
			default:
				AVDECC_ASSERT(false, "Unhandled CounterType");
				return 0;
		}
	}
	static inline CounterType hashToCounterType(size_t const hash)
	{
		static std::unordered_map<size_t, CounterType> s_hashMap{
			{ typeid(StreamOutputCounterValidFlagsMilan12).hash_code(), CounterType::Milan_12 },
			{ typeid(StreamOutputCounterValidFlags17221).hash_code(), CounterType::IEEE17221_2021 },
		};

		if (auto const& it = s_hashMap.find(hash); it != s_hashMap.end())
		{
			return it->second;
		}
		return CounterType::Unknown;
	}

	CounterType _counterType{ CounterType::Unknown };
	size_t _counterTypeHash{ 0 }; // Hash of the currently stored type, used for type checking in template methods
	StreamOutputCounterValidFlags _validFlags{};
	DescriptorCounters _counters{};
};

struct AudioUnitNodeDynamicModel
{
	AvdeccFixedString objectName{};
	SamplingRate currentSamplingRate{};
};

struct StreamNodeDynamicModel
{
	AvdeccFixedString objectName{};
	StreamFormat streamFormat{};
	std::optional<bool> isStreamRunning{ std::nullopt };
	std::optional<StreamDynamicInfo> streamDynamicInfo{ std::nullopt };
};

struct StreamInputNodeDynamicModel : public StreamNodeDynamicModel
{
	model::StreamInputConnectionInfo connectionInfo{};
	std::optional<StreamInputCounters> counters{ std::nullopt };
};

struct StreamOutputNodeDynamicModel : public StreamNodeDynamicModel
{
	model::StreamConnections connections{};
	std::optional<StreamOutputCounters> counters{ std::nullopt };
};

struct JackNodeDynamicModel
{
	AvdeccFixedString objectName{};
};

struct AvbInterfaceNodeDynamicModel
{
	AvdeccFixedString objectName{};
	networkInterface::MacAddress macAddress{};
	UniqueIdentifier clockIdentity{};
	std::uint8_t priority1{ 0xff };
	std::uint8_t clockClass{ 0xff };
	std::uint16_t offsetScaledLogVariance{ 0x0000 };
	std::uint8_t clockAccuracy{ 0xff };
	std::uint8_t priority2{ 0xff };
	std::uint8_t domainNumber{ 0u };
	std::uint8_t logSyncInterval{ 0u };
	std::uint8_t logAnnounceInterval{ 0u };
	std::uint8_t logPDelayInterval{ 0u };
	UniqueIdentifier gptpGrandmasterID{};
	std::uint8_t gptpDomainNumber{ 0u };
	std::optional<AvbInterfaceInfo> avbInterfaceInfo{ std::nullopt };
	std::optional<AsPath> asPath{ std::nullopt };
	std::optional<AvbInterfaceCounters> counters{ std::nullopt };
};

struct ClockSourceNodeDynamicModel
{
	AvdeccFixedString objectName{};
	entity::ClockSourceFlags clockSourceFlags{};
	UniqueIdentifier clockSourceIdentifier{};
};

struct MemoryObjectNodeDynamicModel
{
	AvdeccFixedString objectName{};
	std::uint64_t length{ 0u };
};

//struct LocaleNodeDynamicModel
//{
//};

//struct StringsNodeDynamicModel
//{
//};

struct StreamPortNodeDynamicModel
{
	AudioMappings dynamicAudioMap{};
};

struct AudioClusterNodeDynamicModel
{
	AvdeccFixedString objectName{};
};

//struct AudioMapNodeDynamicModel
//{
//};

struct ControlNodeDynamicModel
{
	AvdeccFixedString objectName{};
	ControlValues values{};
};

struct ClockDomainNodeDynamicModel
{
	AvdeccFixedString objectName{};
	ClockSourceIndex clockSourceIndex{ ClockSourceIndex(0u) };
	std::optional<ClockDomainCounters> counters{ std::nullopt };
	// Milan 1.2 additions
	MediaClockReferenceInfo mediaClockReferenceInfo{};
};

struct TimingNodeDynamicModel
{
	AvdeccFixedString objectName{};
};

struct PtpInstanceNodeDynamicModel
{
	AvdeccFixedString objectName{};
	// TODO: Add PTP_INSTANCE dynamic info - See GET_PTP_INSTANCE_INFO (7.4.82) and other PTP_INSTANCE commands
};

struct PtpPortNodeDynamicModel
{
	AvdeccFixedString objectName{};
	// TODO: Add PTP_PORT dynamic info - See GET_PTP_PORT_INFO (7.4.95) and other PTP_PORT commands
};

struct ConfigurationNodeDynamicModel
{
	AvdeccFixedString objectName{};
	bool isActiveConfiguration{ false };

	// Internal variables
	StringsIndex selectedLocaleBaseIndex{ StringsIndex{ 0u } }; /** Base StringIndex for the selected locale */
	StringsIndex selectedLocaleCountIndexes{ StringsIndex{ 0u } }; /** Count StringIndexes for the selected locale */
	std::unordered_map<StringsIndex, AvdeccFixedString> localizedStrings{}; /** Aggregated copy of all loaded localized strings */
};

struct EntityNodeDynamicModel
{
	AvdeccFixedString entityName{};
	AvdeccFixedString groupName{};
	AvdeccFixedString firmwareVersion{};
	AvdeccFixedString serialNumber{};
	std::uint16_t currentConfiguration{ 0u };
	std::optional<EntityCounters> counters{ std::nullopt };
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
