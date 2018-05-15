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

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/exception.hpp>
#include "avdeccControlledEntityModel.hpp"
#include "exports.hpp"

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
		Exception(Type const type, char const* const text) : la::avdecc::Exception(text), _type(type) {}
		Type getType() const noexcept { return _type; }
	private:
		Type const _type{ Type::None };
	};

	// Getters
	virtual bool gotEnumerationError() const noexcept = 0; // True if the controller had an error during entity model enumeration (leading to Exception::Type::EnumerationError if any throwing method is called).
	virtual bool isAcquired() const noexcept = 0; // Is entity acquired by the controller it's attached to // TODO: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isAcquiring() const noexcept = 0; // Is the attached controller trying to acquire the entity // TODO: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isAcquiredByOther() const noexcept = 0; // Is entity acquired by another controller // TODO: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual bool isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual UniqueIdentifier getOwningControllerID() const noexcept = 0;
	virtual entity::Entity const& getEntity() const noexcept = 0;

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
	operator bool() const noexcept
	{
		return _controlledEntity != nullptr;
	}

	// Default constructor to allow creation of an empty Guard
	ControlledEntityGuard() noexcept
	{
	}

	// Destructor visibility required
	~ControlledEntityGuard()
	{
		if (_controlledEntity)
			_controlledEntity->unlock();
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
	ControlledEntityGuard(SharedControlledEntity entity)
		: _controlledEntity(std::move(entity))//, _lg(*_controlledEntity)
	{
		// TODO: Use the lock_guard, when I'll understand which element is not movable or constructible (probably the ControlledEntity)
		if (_controlledEntity)
			_controlledEntity->lock();
	}

	SharedControlledEntity _controlledEntity{ nullptr };
	//std::lock_guard<ControlledEntity> _lg;
};

} // namespace controller
} // namespace avdecc
} // namespace la
