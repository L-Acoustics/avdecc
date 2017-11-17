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
* @file avdeccControlledEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "avdeccControlledEntityImpl.hpp"
#include <algorithm>
#include <cassert>

namespace la
{
namespace avdecc
{
namespace controller
{

/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
/** Constructor */
ControlledEntityImpl::ControlledEntityImpl(entity::Entity const& entity) noexcept
	: _entity(entity)
{
}

// ControlledEntity overrides
bool ControlledEntityImpl::isAcquired() const noexcept
{
	return _acquireState == AcquireState::Acquired;
}

bool ControlledEntityImpl::isAcquiring() const noexcept
{
	return _acquireState == AcquireState::TryAcquire;
}

bool ControlledEntityImpl::isAcquiredByOther() const noexcept
{
	return _acquireState == AcquireState::AcquiredByOther;
}

UniqueIdentifier ControlledEntityImpl::getOwningControllerID() const noexcept
{
	return _owningControllerID;
}

entity::Entity const& ControlledEntityImpl::getEntity() const noexcept
{
	return _entity;
}

entity::model::EntityDescriptor const& ControlledEntityImpl::getEntityDescriptor() const
{
	return getRootDescriptor<entity::model::EntityDescriptor, &ControlledEntityImpl::_entityDescriptor>();
}

entity::model::ConfigurationDescriptor const& ControlledEntityImpl::getConfigurationDescriptor() const
{
	return getRootDescriptor<entity::model::ConfigurationDescriptor, &ControlledEntityImpl::_configurationDescriptor>();
}

entity::model::LocaleDescriptor const* ControlledEntityImpl::findLocaleDescriptor(std::string const& /*locale*/) const
{
	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	if (_localeDescriptors.empty())
		throw Exception(Exception::Type::InvalidLocaleName, "Entity has no locale");

#pragma message("TBD: Parse 'locale' parameter and find best match")
	// Right now, return the first locale
	return &_localeDescriptors.at(0);
}

entity::model::StreamDescriptor const& ControlledEntityImpl::getStreamInputDescriptor(entity::model::StreamIndex const streamIndex) const
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamInput, streamIndex);

	auto const it = _inputStreams.find(streamIndex);
	if (it == _inputStreams.end())
		throw Exception(Exception::Type::Unknown, "Should be filled with data already");
	return it->second;
}

entity::model::StreamDescriptor const& ControlledEntityImpl::getStreamOutputDescriptor(entity::model::StreamIndex const streamIndex) const
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamOutput, streamIndex);

	auto const it = _outputStreams.find(streamIndex);
	if (it == _outputStreams.end())
		throw Exception(Exception::Type::Unknown, "Should be filled with data already");
	return it->second;
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept
{
	static entity::model::AvdeccFixedString s_noLocalizationString{};
	try
	{
		// Special value meaning NO_STRING
		if (stringReference == 0x1FFF)
			return s_noLocalizationString;

		auto const offset = stringReference >> 3;
		auto const index = stringReference & 0x0007;
		auto const globalOffset = ((offset * 7u) + index) & 0xFFFF;
		return _localizedStrings.at(entity::model::StringsIndex(globalOffset));
	}
	catch (...)
	{
		return s_noLocalizationString;
	}
}

entity::model::StreamConnectedState ControlledEntityImpl::getConnectedSinkState(entity::model::StreamIndex const listenerIndex) const
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamInput, listenerIndex);

	auto const it = _inputStreamStates.find(listenerIndex);
	entity::model::StreamConnectedState state{ getUninitializedIdentifier() , 0, false, false };
	if (it != _inputStreamStates.end())
		state = it->second;
	return state;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamInputAudioMappings(entity::model::StreamIndex const streamIndex) const
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamInput, streamIndex);

	auto const it = _inputStreamAudioMappings.find(streamIndex);
	if (it == _inputStreamAudioMappings.end())
		throw Exception(Exception::Type::Unknown, "Should be filled with data already");
	return it->second;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamOutputAudioMappings(entity::model::StreamIndex const streamIndex) const
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamInput, streamIndex);

	auto const it = _outputStreamAudioMappings.find(streamIndex);
	if (it == _outputStreamAudioMappings.end())
		throw Exception(Exception::Type::Unknown, "Should be filled with data already");
	return it->second;
}

entity::model::EntityDescriptor& ControlledEntityImpl::getEntityDescriptor()
{
	// Implemented over getEntityDescriptor() const overload
	return const_cast<entity::model::EntityDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getEntityDescriptor());
}

entity::model::ConfigurationDescriptor& ControlledEntityImpl::getConfigurationDescriptor()
{
	// Implemented over getConfigurationDescriptor() const overload
	return const_cast<entity::model::ConfigurationDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getConfigurationDescriptor());
}

entity::model::StreamDescriptor& ControlledEntityImpl::getStreamInputDescriptor(entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamInputDescriptor() const overload
	return const_cast<entity::model::StreamDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamInputDescriptor(streamIndex));
}

entity::model::StreamDescriptor& ControlledEntityImpl::getStreamOutputDescriptor(entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamOutputDescriptor() const overload
	return const_cast<entity::model::StreamDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamOutputDescriptor(streamIndex));
}

void ControlledEntityImpl::updateEntity(entity::Entity const& entity) noexcept
{
	_entity = entity;
}

void ControlledEntityImpl::setAcquireState(AcquireState const state) noexcept
{
	_acquireState = state;
}

void ControlledEntityImpl::setOwningController(UniqueIdentifier const controllerID) noexcept
{
	_owningControllerID = controllerID;
}

void ControlledEntityImpl::setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept
{
	_entityDescriptor = descriptor;
	_completedQueries.set(static_cast<std::size_t>(EntityQuery::EntityDescriptor));
}

void ControlledEntityImpl::setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	_configurationDescriptor = descriptor;
	_completedQueries.set(static_cast<std::size_t>(EntityQuery::ConfigurationDescriptor));
}

void ControlledEntityImpl::addInputStreamDescriptor(entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	_inputStreams[streamIndex] = descriptor;
	// Check if we have all input stream descriptors
	if (_configurationDescriptor.descriptorCounts[entity::model::DescriptorType::StreamInput] == _inputStreams.size())
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::InputStreamDescriptor));
	}
}

void ControlledEntityImpl::addOutputStreamDescriptor(entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	_outputStreams[streamIndex] = descriptor;
	// Check if we have all output stream descriptors
	if (_configurationDescriptor.descriptorCounts[entity::model::DescriptorType::StreamOutput] == _outputStreams.size())
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::OutputStreamDescriptor));
	}
}

bool ControlledEntityImpl::addLocaleDescriptor(entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	_localeDescriptors[localeIndex] = descriptor;
	// Check if we have all locale descriptors
	if (_configurationDescriptor.descriptorCounts[entity::model::DescriptorType::Locale] == _localeDescriptors.size())
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::LocaleDescriptor));
		return true;
	}
	return false;
}

void ControlledEntityImpl::addStringsDescriptor(entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept
{
	for (auto strIndex = 0u; strIndex < descriptor.strings.size(); ++strIndex)
	{
		auto localizedStringIndex = entity::model::StringsIndex((stringsIndex - baseStringDescriptorIndex) * descriptor.strings.size() + strIndex);
		_localizedStrings[localizedStringIndex] = descriptor.strings.at(strIndex);
	}
	// Check if we have all strings descriptors
	auto const it = _localeDescriptors.begin(); // We can use any locale descriptor, since all locales must have the same number of strings descriptors
	if (it == _localeDescriptors.end()) // No more local descriptor in the entity, we might be receiving strings after a reset of the entity (it has gone offline then online again)
		return;
	if (_localizedStrings.size() == (it->second.numberOfStringDescriptors * 7u))
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::StringsDescriptor));
	}
}

bool ControlledEntityImpl::setInputStreamState(entity::model::StreamIndex const inputStreamIndex, entity::model::StreamConnectedState const& state) noexcept
{
	auto previous = _inputStreamStates[inputStreamIndex];
	_inputStreamStates[inputStreamIndex] = state;
	// Check if we have all input stream states
	if (_configurationDescriptor.descriptorCounts[entity::model::DescriptorType::StreamInput] == _inputStreamStates.size())
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::InputStreamState));
	}
	return previous != state;
}

bool ControlledEntityImpl::setInputStreamFormat(entity::model::StreamIndex const inputStreamIndex, entity::model::StreamFormat const streamFormat) /* True if format changed */
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamInput, inputStreamIndex);

	auto const it = _inputStreams.find(inputStreamIndex);
	if (it == _inputStreams.end())
		throw Exception(Exception::Type::Unknown, "Should be filled with data already");

	auto& descriptor = it->second;
	auto previous = descriptor.currentFormat;
	descriptor.currentFormat = streamFormat;
	return previous != streamFormat;
}

bool ControlledEntityImpl::setOutputStreamFormat(entity::model::StreamIndex const outputStreamIndex, entity::model::StreamFormat const streamFormat) /* True if format changed */
{
	checkValidStreamIndex(entity::model::DescriptorType::StreamOutput, outputStreamIndex);

	auto const it = _outputStreams.find(outputStreamIndex);
	if (it == _outputStreams.end())
		throw Exception(Exception::Type::Unknown, "Should be filled with data already");

	auto& descriptor = it->second;
	auto previous = descriptor.currentFormat;
	descriptor.currentFormat = streamFormat;
	return previous != streamFormat;
}

void ControlledEntityImpl::clearInputStreamAudioMappings(entity::model::StreamIndex const inputStreamIndex) noexcept
{
	_inputStreamAudioMappings.erase(inputStreamIndex);
}

void ControlledEntityImpl::clearOutputStreamAudioMappings(entity::model::StreamIndex const outputStreamIndex) noexcept
{
	_outputStreamAudioMappings.erase(outputStreamIndex);
}

void ControlledEntityImpl::addInputStreamAudioMappings(entity::model::StreamIndex const inputStreamIndex, entity::model::AudioMappings const& mappings, bool const isComplete) noexcept
{
	auto& audioMappings = _inputStreamAudioMappings[inputStreamIndex];
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(audioMappings.begin(), audioMappings.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		// Not found, add the new mapping
		if (foundIt == audioMappings.end())
			audioMappings.push_back(map);
		else // Otherwise, replace the previous mapping
		{
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
	if (isComplete)
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::InputStreamAudioMappings));
	}
}

void ControlledEntityImpl::addOutputStreamAudioMappings(entity::model::StreamIndex const outputStreamIndex, entity::model::AudioMappings const& mappings, bool const isComplete) noexcept
{
	auto& audioMappings = _outputStreamAudioMappings[outputStreamIndex];
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(audioMappings.begin(), audioMappings.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		// Not found, add the new mapping
		if (foundIt == audioMappings.end())
			audioMappings.push_back(map);
		else // Otherwise, replace the previous mapping
		{
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
	if (isComplete)
	{
		_completedQueries.set(static_cast<std::size_t>(EntityQuery::OutputStreamAudioMappings));
	}
}

void ControlledEntityImpl::removeInputStreamAudioMappings(entity::model::StreamIndex const inputStreamIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& audioMappings = _inputStreamAudioMappings[inputStreamIndex];
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(audioMappings.begin(), audioMappings.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		assert(foundIt != audioMappings.end());
		if (foundIt != audioMappings.end())
			audioMappings.erase(foundIt);
	}
}

void ControlledEntityImpl::removeOutputStreamAudioMappings(entity::model::StreamIndex const outputStreamIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& audioMappings = _outputStreamAudioMappings[outputStreamIndex];
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(audioMappings.begin(), audioMappings.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		assert(foundIt != audioMappings.end());
		if (foundIt != audioMappings.end())
			audioMappings.erase(foundIt);
	}
}

void ControlledEntityImpl::setIgnoreQuery(EntityQuery const query) noexcept
{
	_completedQueries.set(static_cast<std::size_t>(query));
}

void ControlledEntityImpl::setAllIgnored() noexcept
{
	_completedQueries.set();
}

bool ControlledEntityImpl::isQueryComplete(EntityQuery const query) const noexcept
{
	return _completedQueries.test(static_cast<std::size_t>(query));
}

bool ControlledEntityImpl::gotAllQueries() const noexcept
{
	return _completedQueries.all();
}

bool ControlledEntityImpl::wasAdvertised() const noexcept
{
	return _advertised;
}

void ControlledEntityImpl::setAdvertised(bool const wasAdvertised) noexcept
{
	_advertised = wasAdvertised;
}

void ControlledEntityImpl::checkValidStreamIndex(entity::model::DescriptorType const descriptorType, entity::model::StreamIndex const streamIndex) const
{
	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	auto countIt = _configurationDescriptor.descriptorCounts.find(descriptorType);
	if (countIt == _configurationDescriptor.descriptorCounts.end())
		throw Exception(Exception::Type::InvalidStreamIndex, "Invalid stream index");
	if (streamIndex >= countIt->second)
		throw Exception(Exception::Type::InvalidStreamIndex, "Invalid stream index");
}

} // namespace controller
} // namespace avdecc
} // namespace la
