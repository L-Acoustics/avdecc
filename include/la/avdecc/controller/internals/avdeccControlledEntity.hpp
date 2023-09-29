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
* @file avdeccControlledEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc entity controlled by a #la::avdecc::controller::Controller.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/watchDog.hpp>
#include <la/avdecc/internals/exception.hpp>

#include "avdeccControlledEntityModel.hpp"
#include "exports.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>
#include <optional>
#include <map>
#include <set>

namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************************** */
/* ControlledEntity                                                           */
/* ************************************************************************** */
/**
* @brief A local or remote entity that was discovered and is attached to a Controller.
* @details Representation of an entity that was previously discovered by a Controller.
*/
class ControlledEntity
{
public:
	class LA_AVDECC_CONTROLLER_API Exception final : public la::avdecc::Exception
	{
	public:
		enum class Type : std::uint8_t
		{
			None = 0,
			NotSupported, /**< Query not support by the Entity */
			InvalidConfigurationIndex, /**< Specified ConfigurationIndex does not exist */
			InvalidDescriptorIndex, /**< Specified DescriptorIndex (or any derivated) does not exist */
			InvalidLocaleName, /**< Specified Locale does not exist */
			EnumerationError, /**< Trying to get information from an Entity that got an error during descriptors enumeration. Only non-throwing methods can be called. */
			Internal = 99, /**< Internal library error, please report */
		};
		Exception(Type const type, char const* const text)
			: la::avdecc::Exception(text)
			, _type(type)
		{
		}
		Type getType() const noexcept
		{
			return _type;
		}

	private:
		Type const _type{ Type::None };
	};

	/** Compatibility for the Entity */
	enum class CompatibilityFlag : std::uint8_t
	{
		None = 0, /** Not fully IEEE1722.1 compliant entity */

		IEEE17221 = 1u << 0, /** Classic IEEE1722.1 entity */
		Milan = 1u << 1, /** MILAN compatible entity */

		MilanWarning = 1u << 6, /** MILAN compatible entity but with minor warnings in the model/behavior that do not retrograde a Milan entity (this flag it additive with Milan flag) */
		Misbehaving = 1u << 7, /** Entity is sending correctly formed messages but with incoherent values that can cause undefined behavior. */
	};
	using CompatibilityFlags = utils::EnumBitfield<CompatibilityFlag>;

	/** AVB Interface Link Status */
	enum class InterfaceLinkStatus
	{
		Unknown = 0, /** Link status is unknown, might be Up or Down */
		Down = 1, /** Interface is down */
		Up = 2, /** Interface is Up */
	};

	/* Entity Diagnostics */
	struct Diagnostics
	{
		bool redundancyWarning{ false }; /** Flag indicating a Milan redundant device has both interfaces connected to the same network */
		std::set<entity::model::ControlIndex> controlCurrentValueOutOfBounds{}; /** List of Controls whose current value is outside the specified min-max range */
		std::set<entity::model::StreamIndex> streamInputOverLatency{}; /** List of StreamInput whose MSRP Latency is greater than Talker's Presentation Time */
	};

	// Getters
	virtual bool isVirtual() const noexcept = 0; // True if the entity is a virtual one (la::avdecc::controller::Controller methods won't succeed due to the entity not actually been discovered)
	virtual CompatibilityFlags getCompatibilityFlags() const noexcept = 0;
	virtual bool isMilanRedundant() const noexcept = 0; // True if the entity is currently in Milan Redundancy mode (ie. current configuration has at least one redundant stream)
	virtual bool gotFatalEnumerationError() const noexcept = 0; // True if the controller had a fatal error during entity information retrieval (leading to Exception::Type::EnumerationError if any throwing method is called).
	virtual bool isSubscribedToUnsolicitedNotifications() const noexcept = 0;
	virtual bool areUnsolicitedNotificationsSupported() const noexcept = 0;
	virtual bool isAcquired() const noexcept = 0; // Is entity acquired by the controller it's attached to
	virtual bool isAcquireCommandInProgress() const noexcept = 0; // Is the attached controller trying to acquire or release the entity
	virtual bool isAcquiredByOther() const noexcept = 0; // Is entity acquired by another controller
	virtual bool isLocked() const noexcept = 0; // Is entity locked by the controller it's attached to
	virtual bool isLockCommandInProgress() const noexcept = 0; // Is the attached controller trying to lock or unlock the entity
	virtual bool isLockedByOther() const noexcept = 0; // Is entity locked by another controller
	virtual bool isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual bool isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual InterfaceLinkStatus getAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex) const noexcept = 0; // Returns InterfaceLinkStatus::Unknown if EM not supported by the Entity, avbInterfaceIndex does not exist, or the Entity does not support AVB_INTERFACE_COUNTERS
	virtual model::AcquireState getAcquireState() const noexcept = 0;
	virtual UniqueIdentifier getOwningControllerID() const noexcept = 0;
	virtual model::LockState getLockState() const noexcept = 0;
	virtual UniqueIdentifier getLockingControllerID() const noexcept = 0;
	virtual entity::Entity const& getEntity() const noexcept = 0;
	virtual std::optional<entity::model::MilanInfo> getMilanInfo() const noexcept = 0; // Retrieve MilanInfo, guaranteed to be present if CompatibilityFlag::Milan is set
	virtual std::optional<entity::model::ControlIndex> getIdentifyControlIndex() const noexcept = 0; // Retrieve the Identify Control Index, if the entity has a valid one
	virtual bool isEntityModelValidForCaching() const noexcept = 0; // True if the Entity Model is valid for caching
	virtual bool isIdentifying() const noexcept = 0; // True if the Entity is currently identifying itself
	virtual bool hasAnyConfiguration() const noexcept = 0; // True if the Entity has at least one Configuration
	virtual entity::model::ConfigurationIndex getCurrentConfigurationIndex() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity

	// Const Node getters
	virtual model::EntityNode const& getEntityNode() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual model::ConfigurationNode const& getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	virtual model::ConfigurationNode const& getCurrentConfigurationNode() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual model::AudioUnitNode const& getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex do not exist
	virtual model::StreamInputNode const& getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual model::StreamOutputNode const& getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::RedundantStreamNode const& getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if redundantStreamIndex do not exist
	virtual model::RedundantStreamNode const& getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if redundantStreamIndex do not exist
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::JackInputNode const& getJackInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if jackIndex do not exist
	virtual model::JackOutputNode const& getJackOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if jackIndex do not exist
	virtual model::AvbInterfaceNode const& getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if avbInterfaceIndex do not exist
	virtual model::ClockSourceNode const& getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockSourceIndex do not exist
	virtual model::StreamPortNode const& getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex or streamPortIndex do not exist
	virtual model::StreamPortNode const& getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex or streamPortIndex do not exist
	virtual model::AudioClusterNode const& getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if ClusterIndex do not exist
	//virtual model::AudioMapNode const& getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex, streamPortIndex or MapIndex do not exist
	virtual model::ControlNode const& getControlNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if controlIndex do not exist
	virtual model::ClockDomainNode const& getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockDomainIndex do not exist
	virtual model::TimingNode const& getTimingNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if timingIndex do not exist
	virtual model::PtpInstanceNode const& getPtpInstanceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if ptpInstanceIndex do not exist
	virtual model::PtpPortNode const& getPtpPortNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if ptpPortIndex do not exist

	virtual model::LocaleNode const* findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& locale) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::LocalizedStringReference const& stringReference) const noexcept = 0; // Get localized string or empty string if not found, in current configuration descriptor
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const& stringReference) const noexcept = 0; // Get localized string or empty string if not found // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist

	/** Get stream connection information (State and TalkerStream) about a listener's input stream (TalkerStream is meaningful if State is different than NotConnected). */
	virtual entity::model::StreamInputConnectionInfo const& getSinkConnectionInformation(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	/** Get the current AudioMappings for the specified Input StreamPortIndex. Might return redundant mappings as well as primary ones. If you want the non-redundant mappings only, you should use getStreamPortInputNonRedundantAudioMappings instead. */
	virtual entity::model::AudioMappings const& getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	/** Get the current AudioMappings for the specified Input StreamPortIndex. Only return the primary mappings, not the redundant ones. */
	virtual entity::model::AudioMappings getStreamPortInputNonRedundantAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	/** Get the current AudioMappings for the specified Output StreamPortIndex. Might return redundant mappings as well as primary ones. If you want the non-redundant mappings only, you should use getStreamPortOutputNonRedundantAudioMappings instead. */
	virtual entity::model::AudioMappings const& getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	/** Get the current AudioMappings for the specified Output StreamPortIndex. Only return the primary mappings, not the redundant ones. */
	virtual entity::model::AudioMappings getStreamPortOutputNonRedundantAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	/** Get AudioMappings for the specified Input StreamPortIndex that will become invalid for the specified StreamFormat. Might return redundant mappings as well as primary ones. */
	virtual std::map<entity::model::StreamPortIndex, entity::model::AudioMappings> getStreamPortInputInvalidAudioMappingsForStreamFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const = 0; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist

	/** Get connections information about a talker's stream */
	virtual entity::model::StreamConnections const& getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist

	// Statistics
	virtual std::uint64_t getAecpRetryCounter() const noexcept = 0;
	virtual std::uint64_t getAecpTimeoutCounter() const noexcept = 0;
	virtual std::uint64_t getAecpUnexpectedResponseCounter() const noexcept = 0;
	virtual std::chrono::milliseconds const& getAecpResponseAverageTime() const noexcept = 0;
	virtual std::uint64_t getAemAecpUnsolicitedCounter() const noexcept = 0;
	virtual std::uint64_t getAemAecpUnsolicitedLossCounter() const noexcept = 0;
	virtual std::chrono::milliseconds const& getEnumerationTime() const noexcept = 0;

	// Diagnostics
	virtual la::avdecc::controller::ControlledEntity::Diagnostics const& getDiagnostics() const noexcept = 0;

	// Visitor method
	virtual void accept(model::EntityModelVisitor* const visitor, bool const visitAllConfigurations = false) const noexcept = 0;

	/** BasicLockable concept 'lock' method for the whole ControlledEntity */
	virtual void lock() noexcept = 0;
	/** BasicLockable concept 'unlock' method for the whole ControlledEntity */
	virtual void unlock() noexcept = 0;

	// Deleted compiler auto-generated methods
	ControlledEntity(ControlledEntity&&) = delete;
	ControlledEntity(ControlledEntity const&) = delete;
	ControlledEntity& operator=(ControlledEntity const&) = delete;
	ControlledEntity& operator=(ControlledEntity&&) = delete;

protected:
	/** Constructor */
	ControlledEntity() noexcept = default;

	/** Destructor */
	virtual ~ControlledEntity() noexcept = default;
};

using SharedControlledEntity = std::shared_ptr<ControlledEntity>;

/* ************************************************************************** */
/* ControlledEntityGuard                                                      */
/* ************************************************************************** */
/**
* @brief A guard around a ControlledEntity that guarantees it won't be modified while the Guard is alive.
* @warning The guard should not be kept for more than a few milliseconds.
*/
class ControlledEntityGuard final
{
public:
	/** Returns a ControlledEntity const* */
	ControlledEntity const* get() const noexcept
	{
		if (_controlledEntity == nullptr)
			return nullptr;
		return _controlledEntity.get();
	}

	/** Operator to access ControlledEntity */
	ControlledEntity const* operator->() const noexcept
	{
		if (_controlledEntity == nullptr)
			return nullptr;
		return _controlledEntity.operator->();
	}

	/** Operator to access ControlledEntity */
	ControlledEntity const& operator*() const
	{
		if (_controlledEntity == nullptr)
			throw la::avdecc::Exception("ControlledEntity is nullptr");
		return _controlledEntity.operator*();
	}

	/** Returns true if the entity is online (meaning a valid ControlledEntity can be retrieved using an operator overload) */
	bool isValid() const noexcept
	{
		return _controlledEntity != nullptr;
	}

	/** Entity validity bool operator (equivalent to isValid()) */
	explicit operator bool() const noexcept
	{
		return isValid();
	}

	/** Releases the Guarded ControlledEntity (and the exclusive access to it). */
	void reset() noexcept
	{
		unregisterWatchdog();
		unlock();
		_controlledEntity = nullptr;
	}

	// Default constructor to allow creation of an empty Guard
	ControlledEntityGuard() noexcept {}

	// Destructor visibility required
	~ControlledEntityGuard()
	{
		unregisterWatchdog();
		unlock();
	}

	// Swap method
	friend void swap(ControlledEntityGuard& lhs, ControlledEntityGuard& rhs)
	{
		using std::swap;

		// Watchdog 'key' is based on 'this', so when swapping we have to unregister then re-register so the key is changed
		auto const leftHasEntity = !!lhs._controlledEntity;
		auto const rightHasEntity = !!rhs._controlledEntity;
		if (leftHasEntity)
		{
			lhs.unregisterWatchdog();
		}
		if (rightHasEntity)
		{
			rhs.unregisterWatchdog();
		}

		// Swap everything
		swap(lhs._controlledEntity, rhs._controlledEntity);
		swap(lhs._watchDogSharedPointer, rhs._watchDogSharedPointer);
		swap(lhs._watchDog, rhs._watchDog);

		// Re-register
		if (leftHasEntity)
		{
			// But using the new location (lhs swapped with rhs)
			rhs.registerWatchdog();
		}
		if (rightHasEntity)
		{
			// But using the new location (rhs swapped with lhs)
			lhs.registerWatchdog();
		}
	}

	// Allow move semantics
	ControlledEntityGuard(ControlledEntityGuard&& other)
	{
		swap(*this, other);
	}

	ControlledEntityGuard& operator=(ControlledEntityGuard&& other)
	{
		swap(*this, other);
		return *this;
	}

	// Disallow copy
	ControlledEntityGuard(ControlledEntityGuard const&) = delete;
	ControlledEntityGuard& operator=(ControlledEntityGuard const&) = delete;

private:
	friend class ControllerImpl;
	// Ownership (and locked state) is transfered during construction
	ControlledEntityGuard(SharedControlledEntity&& entity)
		: _controlledEntity(std::move(entity))
	{
		registerWatchdog();
	}

	void registerWatchdog() noexcept
	{
		if (_controlledEntity)
		{
			_watchDog.get().registerWatch("avdecc::controller::ControlledEntityGuard::" + utils::toHexString(reinterpret_cast<size_t>(this)), std::chrono::milliseconds{ 500u }, true);
		}
	}

	void unregisterWatchdog() noexcept
	{
		if (_controlledEntity)
		{
			_watchDog.get().unregisterWatch("avdecc::controller::ControlledEntityGuard::" + utils::toHexString(reinterpret_cast<size_t>(this)), true);
		}
	}

	void unlock() noexcept
	{
		if (_controlledEntity)
		{
			// We can unlock, we got ownership (and locked state) during construction
			_controlledEntity->unlock();
		}
	}

	SharedControlledEntity _controlledEntity{ nullptr };
	watchDog::WatchDog::SharedPointer _watchDogSharedPointer{ watchDog::WatchDog::getInstance() };
	std::reference_wrapper<watchDog::WatchDog> _watchDog{ *_watchDogSharedPointer };
};

} // namespace controller
} // namespace avdecc
} // namespace la
