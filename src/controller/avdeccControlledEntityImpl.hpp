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
* @file avdeccControlledEntityImpl.hpp
* @author Christophe Calmejane
*/

#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
#include "avdeccControlledEntityModelTree.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <functional>

namespace la
{
namespace avdecc
{
namespace controller
{

/* ************************************************************ */
/* Private entity::Entity modifiable inherited class            */
/* ************************************************************ */
class ModifiableEntity : public entity::Entity
{
public:
	ModifiableEntity(entity::Entity const& entity)
		: Entity(entity)
	{
	}

	using Entity::setGptpGrandmasterID;
	using Entity::setGptpDomainNumber;
};

/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
class ControlledEntityImpl : public ControlledEntity
{
public:
	enum class DynamicInfoType : std::uint16_t
	{
		// Always required to query (full entity enumeration and dynamic state sync)
		InputStreamAudioMappings,
		OutputStreamAudioMappings,
		InputStreamState,
		OutputStreamState,
		OutputStreamConnection,
		InputStreamInfo,
		OutputStreamInfo,
		GetAvbInfo,
		GetAsPath,
		// Required only when sync'ing dynamic state
		InputStreamFormat,
		OutputStreamFormat,
		EntityName,
		EntityGroupName,
		ConfigurationName,
		InputStreamName,
		OutputStreamName,
		AudioUnitSamplingRate,
		ClockSource,
	};

	using DescriptorKey = std::uint32_t;
	static_assert(sizeof(DescriptorKey) >= sizeof(entity::model::DescriptorType) + sizeof(entity::model::DescriptorIndex), "DescriptorKey size must be greater or equal to DescriptorType + DescriptorIndex");
	using DynamicInfoKey = std::uint64_t;
	static_assert(sizeof(DynamicInfoKey) >= sizeof(DynamicInfoType) + sizeof(entity::model::DescriptorIndex) + sizeof(std::uint16_t), "DynamicInfoKey size must be greater or equal to DynamicInfoType + DescriptorIndex + std::uint16_t");

	/** Constructor */
	ControlledEntityImpl(la::avdecc::entity::Entity const& entity) noexcept;

	// ControlledEntity overrides
	// Getters
	virtual bool gotEnumerationError() const noexcept override;
	virtual bool isAcquired() const noexcept override;
	virtual bool isAcquiring() const noexcept override;
	virtual bool isAcquiredByOther() const noexcept override;
	virtual bool isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual bool isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual UniqueIdentifier getOwningControllerID() const noexcept override;
	virtual entity::Entity const& getEntity() const noexcept override;

	virtual model::EntityNode const& getEntityNode() const override;
	virtual model::ConfigurationNode const& getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex) const override;
	virtual model::ConfigurationNode const& getCurrentConfigurationNode() const override;
	virtual model::StreamInputNode const& getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual model::StreamOutputNode const& getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::RedundantStreamNode const& getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const override;
	virtual model::RedundantStreamNode const& getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const override;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::AudioUnitNode const& getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const override;
	virtual model::ClockSourceNode const& getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const override;
	virtual model::StreamPortNode const& getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const override;
	virtual model::StreamPortNode const& getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const override;
	//virtual model::AudioClusterNode const& getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const override;
	//virtual model::AudioMapNode const& getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const override;
	virtual model::ClockDomainNode const& getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const override;

	virtual model::LocaleNodeStaticModel const* findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& locale) const override; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept override;
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const stringReference) const noexcept override; // Get localized string or empty string if not found // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist

	// Visitor method
	virtual void accept(model::EntityModelVisitor* const visitor) const noexcept override;

	virtual void lock() noexcept override;
	virtual void unlock() noexcept override;

	/** Get connected information about a listener's stream (TalkerID and StreamIndex might be filled even if isConnected is not true, in case of FastConnect) */
	virtual model::StreamConnectionState const& getConnectedSinkState(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	virtual entity::model::AudioMappings const& getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist

	/** Get connections information about a talker's stream */
	virtual model::StreamConnections const& getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist

	// Const getters
	model::EntityDescriptor const& getEntityDescriptor() const; // Throws Exception::NotSupported if EM not supported by the Entity
	model::ConfigurationDescriptor const& getConfigurationDescriptor(entity::model::ConfigurationIndex const configurationIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	model::AudioUnitDescriptor const& getAudioUnitDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex do not exist
	model::StreamInputDescriptor const& getStreamInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	model::StreamOutputDescriptor const& getStreamOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	model::AvbInterfaceDescriptor const& getAvbInterfaceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if avbInterfaceIndex do not exist
	model::ClockSourceDescriptor const& getClockSourceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockSourceIndex do not exist
	model::LocaleDescriptor const& getLocaleDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if localeIndex do not exist
	model::StringsDescriptor const& getStringsDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if stringsIndex do not exist
	model::StreamPortDescriptor const& getStreamPortInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	model::StreamPortDescriptor const& getStreamPortOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	model::AudioClusterDescriptor const& getAudioClusterDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clusterIndex do not exist
	model::AudioMapDescriptor const& getAudioMapDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if mapIndex do not exist
	model::ClockDomainDescriptor const& getClockDomainDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockDomainIndex do not exist

	// Non-const getters
	model::EntityDescriptor& getEntityDescriptor(); // Throws Exception::NotSupported if EM not supported by the Entity
	model::ConfigurationDescriptor& getConfigurationDescriptor(entity::model::ConfigurationIndex const configurationIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	model::AudioUnitDescriptor& getAudioUnitDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if audioUnitIndex do not exist
	model::StreamInputDescriptor& getStreamInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	model::StreamOutputDescriptor& getStreamOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	model::AvbInterfaceDescriptor& getAvbInterfaceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if avbInterfaceIndex do not exist
	model::ClockSourceDescriptor& getClockSourceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockSourceIndex do not exist
	model::LocaleDescriptor& getLocaleDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if localeIndex do not exist
	model::StringsDescriptor& getStringsDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if stringsIndex do not exist
	model::StreamPortDescriptor& getStreamPortInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	model::StreamPortDescriptor& getStreamPortOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	model::AudioClusterDescriptor& getAudioClusterDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clusterIndex do not exist
	model::AudioMapDescriptor& getAudioMapDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if mapIndex do not exist
	model::ClockDomainDescriptor& getClockDomainDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex); // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist // Throws Exception::InvalidDescriptorIndex if clockDomainIndex do not exist

	// Setters (of the model, not the physical entity)
	void setEntity(entity::Entity const& entity) noexcept;
	void setAcquireState(model::AcquireState const state) noexcept; // TODO: This API and all 'acquire' related ones should take a descriptor and index (the acquired state is per branch of the EM)
	void setOwningController(UniqueIdentifier const controllerID) noexcept;
	void setEntityDescriptor(entity::model::EntityDescriptor const& descriptor);
	void setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex);
	void setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex);
	void setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex);
	void setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex);
	void setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex);
	void setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex);
	void setSelectedLocaleBaseIndex(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex);
	void setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex); /* True if all locale descriptors loaded */
	void setStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex);
	void setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex);
	void setStreamPortOutputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex);
	void setAudioClusterDescriptor(entity::model::AudioClusterDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex);
	void setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex);
	void setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex);
	void setInputStreamState(model::StreamConnectionState const& state, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex);
	void clearPortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex);
	void clearPortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex);
	void addPortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings);
	void addPortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings);
	void removePortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings);
	void removePortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings);
	void clearStreamOutputConnections(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex);
	bool addStreamOutputConnection(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream);
	bool delStreamOutputConnection(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream);
	void setInputStreamInfo(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info);
	void setOutputStreamInfo(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info);
	void setAvbInfo(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info);

	// Expected descriptor query methods
	bool checkAndClearExpectedDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	void setDescriptorExpected(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	bool gotAllExpectedDescriptors() const noexcept;
	void clearExpectedDescriptors(entity::model::ConfigurationIndex const configurationIndex) noexcept;

	// Expected dynamic info query methods
	bool checkAndClearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex = 0u) noexcept;
	void setDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex = 0u) noexcept;
	bool gotAllExpectedDynamicInfo() const noexcept;
	void clearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex) noexcept;

	void setEnumerationError(bool const gotEnumerationError) noexcept;
	bool wasAdvertised() const noexcept;
	void setAdvertised(bool const wasAdvertised) noexcept;

	// Defaulted compiler auto-generated methods
	ControlledEntityImpl(ControlledEntityImpl&&) = default;
	ControlledEntityImpl(ControlledEntityImpl const&) = default;
	ControlledEntityImpl& operator=(ControlledEntityImpl const&) = default;
	ControlledEntityImpl& operator=(ControlledEntityImpl&&) = default;

protected:
	template<class NodeType, typename = std::enable_if_t<std::is_base_of<model::Node, NodeType>::value>>
	static constexpr size_t getHashCode(NodeType const* const node) noexcept
	{
		return typeid(decltype(node)).hash_code();
	}
	template<class NodeType, typename = std::enable_if_t<std::is_base_of<model::Node, NodeType>::value>>
	static constexpr size_t getHashCode() noexcept
	{
		return typeid(NodeType const*).hash_code();
	}

	template<class NodeType, typename = std::enable_if_t<std::is_base_of<model::VirtualNode, NodeType>::value>>
	static void initNode(NodeType& node, entity::model::DescriptorType const descriptorType) noexcept
	{
		node.descriptorType = descriptorType;
	}

	template<class NodeType, typename = std::enable_if_t<std::is_base_of<model::EntityModelNode, NodeType>::value>>
	static void initNode(NodeType& node, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, model::AcquireState const acquireState) noexcept
	{
		node.descriptorType = descriptorType;
		node.descriptorIndex = descriptorIndex;
		node.acquireState = acquireState;
	}

	template<class NodeType, typename = std::enable_if_t<std::is_base_of<model::VirtualNode, NodeType>::value>>
	static void initNode(NodeType& node, entity::model::DescriptorType const descriptorType, model::VirtualIndex const virtualIndex) noexcept
	{
		node.descriptorType = descriptorType;
		node.virtualIndex = virtualIndex;
	}

private:
	void checkAndBuildEntityModelGraph() const noexcept;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	void buildRedundancyNodes(model::ConfigurationNode& configNode) const noexcept;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Private variables
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DescriptorKey>> _expectedDescriptors{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DynamicInfoKey>> _expectedDynamicInfo{};
	bool _enumerateError{ false }; // Have we got an error during entity enumeration
	bool _advertised{ false }; // Has the entity been advertised to the observers
	model::AcquireState _acquireState{ model::AcquireState::Undefined }; // TODO: Should be a graph of descriptors
	UniqueIdentifier _owningControllerID{ getUninitializedIdentifier() }; // EID of the controller currently owning (who acquired) this entity
	// Entity variables
	ModifiableEntity _entity; // No NSMI, Entity has no default constructor but it has to be passed to the only constructor of this class anyway
	// Entity Model
	mutable model::EntityDescriptor _entityModel{}; // Model as represented by the AVDECC protocol
	mutable model::EntityNode _entityNode{}; // Model as represented by the ControlledEntity
};

} // namespace controller
} // namespace avdecc
} // namespace la
