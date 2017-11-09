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
* @file avdeccControlledEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc entity controlled by a #la::avdecc::controller::Controller.
*/

#pragma once

#include <string>
#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/exception.hpp>
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
		enum class Type
		{
			Unknown = 0,
			NotSupported = 1,
			InvalidStreamIndex = 2,
			InvalidLocaleName = 3,
		};
		Exception(Type const type, char const* const text) : la::avdecc::Exception(text), _type(type) {}
		Type getType() const noexcept { return _type; }
	private:
		Type const _type{ Type::Unknown };
	};

	enum class EntityQuery
	{
		EntityDescriptor = 0,
		ConfigurationDescriptor = 1,
		LocaleDescriptor = 2,
		StringsDescriptor = 3,
		InputStreamDescriptor = 4,
		OutputStreamDescriptor = 5,
		InputStreamState = 6,
		InputStreamAudioMappings = 7,
		OutputStreamAudioMappings = 8,

		CountQueries = 9 /**< Count of EntityQuery elements, to be increased if a new value is added */
	};

	enum class AcquireState
	{
		Undefined,
		NotAcquired,
		TryAcquire,
		Acquired,
		AcquiredByOther,
	};

	// Getters
	virtual bool isAcquired() const noexcept = 0; // Is entity acquired by the controller it's attached to // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isAcquiring() const noexcept = 0; // Is the attached controller trying to acquire the entity // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isAcquiredByOther() const noexcept = 0; // Is entity acquired by another controller // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual UniqueIdentifier getOwningControllerID() const noexcept = 0;
	virtual entity::Entity const& getEntity() const noexcept = 0;
	virtual entity::model::EntityDescriptor const& getEntityDescriptor() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual entity::model::ConfigurationDescriptor const& getConfigurationDescriptor() const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual entity::model::LocaleDescriptor const* findLocaleDescriptor(std::string const& locale) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual entity::model::StreamDescriptor const& getStreamInputDescriptor(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::StreamDescriptor const& getStreamOutputDescriptor(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept = 0;
	/** Get connected information about a listener's stream (TalkerID and StreamIndex might be filled even if isConnected is not true, in case of FastConnect) */
	virtual entity::model::StreamConnectedState getConnectedSinkState(entity::model::StreamIndex const listenerIndex) const = 0; // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamInputAudioMappings(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamOutputAudioMappings(entity::model::StreamIndex const streamIndex) const = 0; // Throws Exception::InvalidStreamIndex if streamIndex do not exist

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

} // namespace controller
} // namespace avdecc
} // namespace la
