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
* @file avdeccControlledEntityImpl.hpp
* @author Christophe Calmejane
*/

#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
#include <string>
#include <unordered_map>
#include <bitset>
#include <functional>

namespace la
{
namespace avdecc
{
namespace controller
{

/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
class ControlledEntityImpl final : public ControlledEntity
{
public:
	using UniquePointer = std::unique_ptr<ControlledEntityImpl>;

	/** Constructor */
	ControlledEntityImpl(la::avdecc::entity::Entity const& entity) noexcept;

	// ControlledEntity overrides
	virtual bool isAcquired() const noexcept override; // Is entity acquired by the controller it's attached to // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isAcquiring() const noexcept override; // Is the attached controller trying to acquire the entity // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual bool isAcquiredByOther() const noexcept override; // Is entity acquired by another controller // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	virtual UniqueIdentifier getOwningControllerID() const noexcept override;
	virtual entity::Entity const& getEntity() const noexcept override;
	virtual entity::model::EntityDescriptor const& getEntityDescriptor() const override; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual entity::model::ConfigurationDescriptor const& getConfigurationDescriptor() const override; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual entity::model::LocaleDescriptor const* findLocaleDescriptor(std::string const& locale) const override; // Throws Exception::NotSupported if EM not supported by the Entity
	virtual entity::model::StreamDescriptor const& getStreamInputDescriptor(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::StreamDescriptor const& getStreamOutputDescriptor(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept override;
	/** Get connected information about a listener's stream (TalkerID and StreamIndex might be filled even if isConnected is not true, in case of FastConnect) */
	virtual entity::model::StreamConnectedState getConnectedSinkState(entity::model::StreamIndex const listenerIndex) const override; // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamInputAudioMappings(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::InvalidStreamIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamOutputAudioMappings(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::InvalidStreamIndex if streamIndex do not exist

	// Non-const getters
	entity::model::EntityDescriptor& getEntityDescriptor(); // Throws Exception::NotSupported if EM not supported by the Entity
	entity::model::ConfigurationDescriptor& getConfigurationDescriptor(); // Throws Exception::NotSupported if EM not supported by the Entity

	// Setters (of the model, not the physical entity)
	void updateEntity(entity::Entity const& entity) noexcept;
	void setAcquireState(AcquireState const state) noexcept; // TBD: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	void setOwningController(UniqueIdentifier const controllerID) noexcept;
	void setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept;
	void setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor) noexcept;
	bool addLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor) noexcept; /* True if all locale descriptors loaded */
	void addStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept;
	void addInputStreamDescriptor(entity::model::StreamDescriptor const& descriptor) noexcept;
	void addOutputStreamDescriptor(entity::model::StreamDescriptor const& descriptor) noexcept;
	bool setInputStreamState(entity::model::StreamIndex const inputStreamIndex, entity::model::StreamConnectedState const& state) noexcept; /* True if state changed */
	bool setInputStreamFormat(entity::model::StreamIndex const inputStreamIndex, entity::model::StreamFormat const streamFormat); /* True if format changed */
	bool setOutputStreamFormat(entity::model::StreamIndex const outputStreamIndex, entity::model::StreamFormat const streamFormat); /* True if format changed */
	void clearInputStreamAudioMappings(entity::model::StreamIndex const inputStreamIndex) noexcept;
	void clearOutputStreamAudioMappings(entity::model::StreamIndex const outputStreamIndex) noexcept;
	void addInputStreamAudioMappings(entity::model::StreamIndex const inputStreamIndex, entity::model::AudioMappings const& mappings, bool const isComplete = false) noexcept;
	void addOutputStreamAudioMappings(entity::model::StreamIndex const outputStreamIndex, entity::model::AudioMappings const& mappings, bool const isComplete = false) noexcept;
	void removeInputStreamAudioMappings(entity::model::StreamIndex const inputStreamIndex, entity::model::AudioMappings const& mappings) noexcept;
	void removeOutputStreamAudioMappings(entity::model::StreamIndex const outputStreamIndex, entity::model::AudioMappings const& mappings) noexcept;

	void setIgnoreQuery(EntityQuery const query) noexcept;
	void setAllIgnored() noexcept;
	bool isQueryComplete(EntityQuery const query) const noexcept;
	bool gotAllQueries() const noexcept;
	bool wasAdvertised() const noexcept;
	void setAdvertised(bool const wasAdvertised) noexcept;

	// Defaulted compiler auto-generated methods
	ControlledEntityImpl(ControlledEntityImpl&&) = default;
	ControlledEntityImpl(ControlledEntityImpl const&) = default;
	ControlledEntityImpl& operator=(ControlledEntityImpl const&) = default;
	ControlledEntityImpl& operator=(ControlledEntityImpl&&) = default;

private:
	template<typename DescriptorType, DescriptorType ControlledEntityImpl::*MemberPtr>
	DescriptorType const& getRootDescriptor() const
	{
		if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
			throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

		return this->*MemberPtr;
	}
	void checkValidStreamIndex(entity::model::DescriptorType const descriptorType, entity::model::StreamIndex const streamIndex) const;

	// Controller variables
	std::bitset<static_cast<std::size_t>(EntityQuery::CountQueries)> _completedQueries{};
	bool _advertised{ false };
	AcquireState _acquireState{ AcquireState::Undefined }; // TBD: Should be a graph of descriptors
	UniqueIdentifier _owningControllerID{ getUninitializedIdentifier() }; // EID of the controller currently owning (who acquired) this entity
	// Entity variables
	entity::Entity _entity; // No NSMI, Entity has no default constructor but it has to be passed to the only constructor or this class anyway
	// Entity Model
	entity::model::EntityDescriptor _entityDescriptor{};
	entity::model::ConfigurationDescriptor _configurationDescriptor{};
	std::unordered_map<entity::model::LocaleIndex, entity::model::LocaleDescriptor> _localeDescriptors{};
	std::unordered_map<entity::model::StringsIndex, entity::model::AvdeccFixedString> _localizedStrings{}; // Localized strings for selected locale
	std::unordered_map<entity::model::StreamIndex, entity::model::StreamDescriptor> _inputStreams{};
	std::unordered_map<entity::model::StreamIndex, entity::model::StreamDescriptor> _outputStreams{};
	std::unordered_map<entity::model::StreamIndex, entity::model::StreamConnectedState> _inputStreamStates{};
	std::unordered_map<entity::model::StreamIndex, entity::model::AudioMappings> _inputStreamAudioMappings{};
	std::unordered_map<entity::model::StreamIndex, entity::model::AudioMappings> _outputStreamAudioMappings{};
};

} // namespace controller
} // namespace avdecc
} // namespace la
