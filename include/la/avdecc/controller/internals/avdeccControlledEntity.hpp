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
* @file avdeccControlledEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc entity controlled by a #la::avdecc::controller::Controller.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/internals/exception.hpp>
#include <la/avdecc/internals/watchDog.hpp>
#include "avdeccControlledEntityModel.hpp"
#include "exports.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>
#include <optional>

namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************************** */
/* ControlledEntity                                                           */
/* ************************************************************************** */
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

		Misbehaving = 1u << 7, /** Entity is sending correctly formed messages but with incoherent values that can cause undefined behavior. */
	};
	using CompatibilityFlags = EnumBitfield<CompatibilityFlag>;

	/** AVB Interface Link Status */
	enum class InterfaceLinkStatus
	{
		Unknown = 0, /** Link status is unknown, might be Up or Down */
		Down = 1, /** Interface is down */
		Up = 2, /** Interface is Up */
	};

	// Getters
	virtual CompatibilityFlags getCompatibilityFlags() const noexcept = 0;
	virtual bool gotFatalEnumerationError() const noexcept = 0; // True if the controller had a fatal error during entity information retrieval (leading to Exception::Type::EnumerationError if any throwing method is called).
	virtual bool isSubscribedToUnsolicitedNotifications() const noexcept = 0;
	virtual bool isAcquired() const noexcept = 0; // Is entity acquired by the controller it's attached to
	virtual bool isAcquiring() const noexcept = 0; // Is the attached controller trying to acquire the entity
	virtual bool isAcquiredByOther() const noexcept = 0; // Is entity acquired by another controller
	virtual bool isLocked() const noexcept = 0; // Is entity locked by the controller it's attached to
	virtual bool isLocking() const noexcept = 0; // Is the attached controller trying to lock the entity
	virtual bool isLockedByOther() const noexcept = 0; // Is entity locked by another controller
	virtual bool isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual bool isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual InterfaceLinkStatus getAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex) const = 0;
	virtual model::AcquireState getAcquireState() const noexcept = 0;
	virtual UniqueIdentifier getOwningControllerID() const noexcept = 0;
	virtual model::LockState getLockState() const noexcept = 0;
	virtual UniqueIdentifier getLockingControllerID() const noexcept = 0;
	virtual entity::Entity const& getEntity() const noexcept = 0;
	virtual entity::model::MilanInfo const& getMilanInfo() const noexcept = 0;

	virtual model::EntityNode const& getEntityNode() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual model::ConfigurationNode const& getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	virtual model::ConfigurationNode const& getCurrentConfigurationNode() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual model::StreamInputNode const& getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual model::StreamOutputNode const& getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::RedundantStreamNode const& getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if redundantStreamIndex do not exist
	virtual model::RedundantStreamNode const& getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if redundantStreamIndex do not exist
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::AudioUnitNode const& getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex do not exist
	virtual model::AvbInterfaceNode const& getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if avbInterfaceIndex do not exist
	virtual model::ClockSourceNode const& getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockSourceIndex do not exist
	virtual model::StreamPortNode const& getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex or streamPortIndex do not exist
	virtual model::StreamPortNode const& getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex or streamPortIndex do not exist
	//virtual model::AudioClusterNode const& getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex, streamPortIndex or ClusterIndex do not exist
	//virtual model::AudioMapNode const& getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex, streamPortIndex or MapIndex do not exist
	virtual model::ClockDomainNode const& getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockDomainIndex do not exist

	virtual model::LocaleNodeStaticModel const* findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& locale) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept = 0; // Get localized string or empty string if not found, in current configuration descriptor
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const stringReference) const noexcept = 0; // Get localized string or empty string if not found // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist

	/** Get connected information about a listener's stream (TalkerID and StreamIndex might be filled even if isConnected is not true, in case of FastConnect) */
	virtual model::StreamConnectionState const& getConnectedSinkState(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	virtual entity::model::AudioMappings const& getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist

	/** Get connections information about a talker's stream */
	virtual model::StreamConnections const& getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist

	// Visitor method
	virtual void accept(model::EntityModelVisitor* const visitor) const noexcept = 0;

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

/* ************************************************************************** */
/* ControlledEntityGuard                                                      */
/* ************************************************************************** */
/** A guard around a ControlledEntity that guarantees it won't be modified while the Guard is alive. WARNING: The guard should not be kept for more than a few milliseconds. */
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
	explicit operator bool() const noexcept
	{
		return _controlledEntity != nullptr;
	}

	/** Releases the Guarded ControlledEntity (and the exclusive access to it). */
	void reset() noexcept
	{
		unlock();
		_controlledEntity = nullptr;
	}

	// Default constructor to allow creation of an empty Guard
	ControlledEntityGuard() noexcept {}

	// Destructor visibility required
	~ControlledEntityGuard()
	{
		unlock();
	}

	// Allow move semantics
	ControlledEntityGuard(ControlledEntityGuard&&) = default;
	ControlledEntityGuard& operator=(ControlledEntityGuard&&) = default;

	// Disallow copy
	ControlledEntityGuard(ControlledEntityGuard const&) = delete;
	ControlledEntityGuard& operator=(ControlledEntityGuard const&) = delete;

private:
	friend class ControllerImpl;
	using SharedControlledEntity = std::shared_ptr<ControlledEntity>;
	// Ownership (and locked state) is transfered during construction
	ControlledEntityGuard(SharedControlledEntity&& entity)
		: _controlledEntity(std::move(entity))
	{
		if (_controlledEntity)
		{
			la::avdecc::watchDog::WatchDog::getInstance().registerWatch("avdecc::controller::ControlledEntityGuard::" + toHexString(reinterpret_cast<size_t>(this)), std::chrono::milliseconds{ 500u });
		}
	}

	void unlock() noexcept
	{
		if (_controlledEntity)
		{
			la::avdecc::watchDog::WatchDog::getInstance().unregisterWatch("avdecc::controller::ControlledEntityGuard::" + toHexString(reinterpret_cast<size_t>(this)));
			// We can unlock, we got ownership (and locked state) during construction
			_controlledEntity->unlock();
		}
	}

	SharedControlledEntity _controlledEntity{ nullptr };
};

} // namespace controller
} // namespace avdecc
} // namespace la
