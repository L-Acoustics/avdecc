/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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

#pragma once

#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
#include "avdeccControlledEntityModelTree.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <functional>
#include <chrono>
#include <mutex>
#include <utility>
#include <thread>

namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
class ControlledEntityImpl : public ControlledEntity
{
public:
	/** Lock Information that is shared among all ControlledEntities */
	struct LockInformation
	{
		using SharedPointer = std::shared_ptr<LockInformation>;

		std::recursive_mutex lock{};
		std::uint32_t lockedCount{ 0u };
		std::thread::id lockingThreadID{};
	};

	enum class EnumerationSteps : std::uint16_t
	{
		None = 0,
		GetMilanInfo = 1u << 0,
		RegisterUnsol = 1u << 1,
		GetStaticModel = 1u << 2,
		GetDescriptorDynamicInfo = 1u << 3, /** DescriptorDynamicInfoType */
		GetDynamicInfo = 1u << 4, /** DynamicInfoType */
	};

	/** Milan Vendor Unique Information */
	enum class MilanInfoType : std::uint16_t
	{
		MilanInfo, // GET_MILAN_INFO
	};

	/** Dynamic information to retrieve from entities. This is always required, either from a first enumeration or from recover from loss of unsolicited notification. */
	enum class DynamicInfoType : std::uint16_t
	{
		AcquiredState, // acquireEntity(ReleasedFlag)
		LockedState, // lockEntity(ReleasedFlag)
		InputStreamAudioMappings, // getStreamPortInputAudioMap (GET_AUDIO_MAP)
		OutputStreamAudioMappings, // getStreamPortOutputAudioMap (GET_AUDIO_MAP)
		InputStreamState, // getListenerStreamState (GET_RX_STATE)
		OutputStreamState, // getTalkerStreamState (GET_TX_STATE)
		OutputStreamConnection, // getTalkerStreamConnection (GET_TX_CONNECTION)
		InputStreamInfo, // getStreamInputInfo (GET_STREAM_INFO)
		OutputStreamInfo, // getStreamOutputInfo (GET_STREAM_INFO)
		GetAvbInfo, // getAvbInfo (GET_AVB_INFO)
		GetAsPath, // getAsPath (GET_AS_PATH)
		GetAvbInterfaceCounters, // getAvbInterfaceCounters (GET_COUNTERS)
		GetClockDomainCounters, // getClockDomainCounters (GET_COUNTERS)
		GetStreamInputCounters, // getStreamInputCounters (GET_COUNTERS)
	};

	/** Dynamic information stored in descriptors. Only required to retrieve from entities when the static model is known (because it was in EntityModelID cache).  */
	enum class DescriptorDynamicInfoType : std::uint16_t
	{
		ConfigurationName, // CONFIGURATION.object_name -> GET_NAME (7.4.18)
		AudioUnitName, // AUDIO_UNIT.object_name -> GET_NAME (7.4.18)
		AudioUnitSamplingRate, // AUDIO_UNIT.current_sampling_rate GET_SAMPLING_RATE (7.4.22)
		InputStreamName, // STREAM_INPUT.object_name -> GET_NAME (7.4.18)
		InputStreamFormat, // STREAM_INPUT.current_format -> GET_STREAM_FORMAT (7.4.10)
		OutputStreamName, // STREAM_OUTPUT.object_name -> GET_NAME (7.4.18)
		OutputStreamFormat, // STREAM_OUTPUT.current_format -> GET_STREAM_FORMAT (7.4.10)
		AvbInterfaceName, // AVB_INTERFACE.object_name -> GET_NAME (7.4.18)
		ClockSourceName, // CLOCK_SOURCE.object_name -> GET_NAME (7.4.18)
		MemoryObjectName, // MEMORY_OBJECT.object_name -> GET_NAME (7.4.18)
		MemoryObjectLength, // MEMORY_OBJECT.length -> GET_MEMORY_OBJECT_LENGTH (7.4.73)
		AudioClusterName, // AUDIO_CLUSTER.object_name -> GET_NAME (7.4.18)
		ClockDomainName, // CLOCK_DOMAIN.object_name -> GET_NAME (7.4.18)
		ClockDomainSourceIndex, // CLOCK_DOMAIN.clock_source_index -> GET_CLOCK_SOURCE (7.4.24)
	};

	using MilanInfoKey = std::underlying_type_t<MilanInfoType>;
	using DescriptorKey = std::uint32_t;
	static_assert(sizeof(DescriptorKey) >= sizeof(entity::model::DescriptorType) + sizeof(entity::model::DescriptorIndex), "DescriptorKey size must be greater or equal to DescriptorType + DescriptorIndex");
	using DynamicInfoKey = std::uint64_t;
	static_assert(sizeof(DynamicInfoKey) >= sizeof(DynamicInfoType) + sizeof(entity::model::DescriptorIndex) + sizeof(std::uint16_t), "DynamicInfoKey size must be greater or equal to DynamicInfoType + DescriptorIndex + std::uint16_t");
	using DescriptorDynamicInfoKey = std::uint64_t;
	static_assert(sizeof(DescriptorDynamicInfoKey) >= sizeof(DescriptorDynamicInfoType) + sizeof(entity::model::DescriptorIndex), "DescriptorDynamicInfoKey size must be greater or equal to DescriptorDynamicInfoType + DescriptorIndex");

	/** Constructor */
	ControlledEntityImpl(la::avdecc::entity::Entity const& entity, LockInformation::SharedPointer const& sharedLock) noexcept;

	// ControlledEntity overrides
	// Getters
	virtual CompatibilityFlags getCompatibilityFlags() const noexcept override;
	virtual bool gotFatalEnumerationError() const noexcept override;
	virtual bool isSubscribedToUnsolicitedNotifications() const noexcept override;
	virtual bool isAcquired() const noexcept override;
	virtual bool isAcquiring() const noexcept override;
	virtual bool isAcquiredByOther() const noexcept override;
	virtual bool isLocked() const noexcept override;
	virtual bool isLocking() const noexcept override;
	virtual bool isLockedByOther() const noexcept override;
	virtual bool isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual bool isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual InterfaceLinkStatus getAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex) const override;
	virtual model::AcquireState getAcquireState() const noexcept override;
	virtual UniqueIdentifier getOwningControllerID() const noexcept override;
	virtual model::LockState getLockState() const noexcept override;
	virtual UniqueIdentifier getLockingControllerID() const noexcept override;
	virtual entity::Entity const& getEntity() const noexcept override;
	virtual entity::model::MilanInfo const& getMilanInfo() const noexcept override;

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
	virtual model::AvbInterfaceNode const& getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const override;
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

	// Const Tree getters, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist
	model::EntityStaticTree const& getEntityStaticTree() const;
	model::EntityDynamicTree const& getEntityDynamicTree() const;
	model::ConfigurationStaticTree const& getConfigurationStaticTree(entity::model::ConfigurationIndex const configurationIndex) const;
	model::ConfigurationDynamicTree const& getConfigurationDynamicTree(entity::model::ConfigurationIndex const configurationIndex) const;
	entity::model::ConfigurationIndex getCurrentConfigurationIndex() const noexcept;

	// Const NodeModel getters, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist, Exception::InvalidDescriptorIndex if descriptorIndex is invalid
	model::EntityNodeStaticModel const& getEntityNodeStaticModel() const;
	model::EntityNodeDynamicModel const& getEntityNodeDynamicModel() const;
	model::ConfigurationNodeStaticModel const& getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex) const;
	model::ConfigurationNodeDynamicModel const& getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex) const;
	template<typename FieldPointer, typename DescriptorIndexType>
	typename std::remove_pointer_t<FieldPointer>::mapped_type const& getNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const index, FieldPointer model::ConfigurationStaticTree::*Field) const
	{
		auto const& configStaticTree = getConfigurationStaticTree(configurationIndex);

		auto const it = (configStaticTree.*Field).find(index);
		if (it == (configStaticTree.*Field).end())
			throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid index");

		return it->second;
	}
	template<typename FieldPointer, typename DescriptorIndexType>
	typename std::remove_pointer_t<FieldPointer>::mapped_type const& getNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const index, FieldPointer model::ConfigurationDynamicTree::*Field) const
	{
		auto const& configDynamicTree = getConfigurationDynamicTree(configurationIndex);

		auto const it = (configDynamicTree.*Field).find(index);
		if (it == (configDynamicTree.*Field).end())
			throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid index");

		return it->second;
	}

	// Non-const Tree getters
	model::EntityStaticTree& getEntityStaticTree() noexcept;
	model::EntityDynamicTree& getEntityDynamicTree() noexcept;
	model::ConfigurationStaticTree& getConfigurationStaticTree(entity::model::ConfigurationIndex const configurationIndex) noexcept;
	model::ConfigurationDynamicTree& getConfigurationDynamicTree(entity::model::ConfigurationIndex const configurationIndex) noexcept;

	// Non-const NodeModel getters
	model::EntityNodeStaticModel& getEntityNodeStaticModel() noexcept;
	model::EntityNodeDynamicModel& getEntityNodeDynamicModel() noexcept;
	model::ConfigurationNodeStaticModel& getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex) noexcept;
	model::ConfigurationNodeDynamicModel& getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex) noexcept;
	template<typename FieldPointer, typename DescriptorIndexType>
	typename std::remove_pointer_t<FieldPointer>::mapped_type& getNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const index, FieldPointer model::ConfigurationStaticTree::*Field) noexcept
	{
		AVDECC_ASSERT(_sharedLock->lockedCount >= 0, "ControlledEntity should be locked");

		auto& configStaticTree = getConfigurationStaticTree(configurationIndex);
		return (configStaticTree.*Field)[index];
	}
	template<typename FieldPointer, typename DescriptorIndexType>
	typename std::remove_pointer_t<FieldPointer>::mapped_type& getNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const index, FieldPointer model::ConfigurationDynamicTree::*Field) noexcept
	{
		AVDECC_ASSERT(_sharedLock->lockedCount >= 0, "ControlledEntity should be locked");

		auto& configDynamicTree = getConfigurationDynamicTree(configurationIndex);
		return (configDynamicTree.*Field)[index];
	}

	// Setters of the DescriptorDynamic info, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist, Exception::InvalidDescriptorIndex if descriptorIndex is invalid
	void setEntityName(entity::model::AvdeccFixedString const& name) noexcept;
	void setEntityGroupName(entity::model::AvdeccFixedString const& name) noexcept;
	void setCurrentConfiguration(entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void setConfigurationName(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name) noexcept;
	template<typename FieldPointer, typename DescriptorIndexType>
	void setObjectName(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const index, FieldPointer model::ConfigurationDynamicTree::*Field, entity::model::AvdeccFixedString const& name) noexcept
	{
		auto& dynamicModel = getNodeDynamicModel(configurationIndex, index, Field);
		dynamicModel.objectName = name;
	}
	void setSamplingRate(entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept;
	void setStreamInputFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept;
	model::StreamConnectionState setStreamInputConnectionState(entity::model::StreamIndex const streamIndex, model::StreamConnectionState const& state) noexcept;
	entity::model::StreamInfo setStreamInputInfo(entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept; // Returns previous StreamInfo
	void setStreamOutputFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept;
	void clearStreamOutputConnections(entity::model::StreamIndex const streamIndex) noexcept;
	bool addStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream) noexcept; // Returns true if effectively added
	bool delStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream) noexcept; // Returns true if effectively removed
	entity::model::StreamInfo setStreamOutputInfo(entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept; // Returns previous StreamInfo
	entity::model::AvbInfo setAvbInfo(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept; // Returns previous AvbInfo
	entity::model::AsPath setAsPath(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath) noexcept; // Returns previous AsPath
	void setSelectedLocaleBaseIndex(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex) noexcept;
	void clearStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) noexcept;
	void addStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept;
	void removeStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept;
	void clearStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) noexcept;
	void addStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept;
	void removeStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept;
	void setClockSource(entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept;
	void setMemoryObjectLength(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept;
	model::AvbInterfaceCounters& getAvbInterfaceCounters(entity::model::AvbInterfaceIndex const avbInterfaceIndex) noexcept;
	model::ClockDomainCounters& getClockDomainCounters(entity::model::ClockDomainIndex const clockDomainIndex) noexcept;
	model::StreamInputCounters& getStreamInputCounters(entity::model::StreamIndex const streamIndex) noexcept;

	// Setters (of the model, not the physical entity)
	void setEntity(entity::Entity const& entity) noexcept;
	InterfaceLinkStatus setAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex, InterfaceLinkStatus const linkStatus) noexcept; // Returns previous link status
	void setAcquireState(model::AcquireState const state) noexcept;
	void setOwningController(UniqueIdentifier const controllerID) noexcept;
	void setLockState(model::LockState const state) noexcept;
	void setLockingController(UniqueIdentifier const controllerID) noexcept;
	void setMilanInfo(entity::model::MilanInfo const& info) noexcept;

	// Setters of the Model from AEM Descriptors (including DescriptorDynamic info)
	bool setCachedEntityStaticTree(model::EntityStaticTree const& cachedStaticTree, entity::model::EntityDescriptor const& descriptor) noexcept; // Returns true if the cached EntityStaticTree is accepted (and set) for this entity
	void setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept;
	void setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) noexcept;
	void setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept;
	void setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept;
	void setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex) noexcept;
	void setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex) noexcept;
	void setMemoryObjectDescriptor(entity::model::MemoryObjectDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex) noexcept;
	void setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex) noexcept;
	void setStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex) noexcept;
	void setLocalizedStrings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, model::AvdeccFixedStrings const& strings) noexcept;
	void setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept;
	void setStreamPortOutputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept;
	void setAudioClusterDescriptor(entity::model::AudioClusterDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) noexcept;
	void setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) noexcept;
	void setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) noexcept;

	// Expected Milan info query methods
	bool checkAndClearExpectedMilanInfo(MilanInfoType const milanInfoType) noexcept;
	void setMilanInfoExpected(MilanInfoType const milanInfoType) noexcept;
	bool gotAllExpectedMilanInfo() const noexcept;
	std::pair<bool, std::chrono::milliseconds> getQueryMilanInfoRetryTimer() noexcept;

	// Expected descriptor query methods
	bool checkAndClearExpectedDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	void setDescriptorExpected(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	bool gotAllExpectedDescriptors() const noexcept;
	std::pair<bool, std::chrono::milliseconds> getQueryDescriptorRetryTimer() noexcept;

	// Expected dynamic info query methods
	bool checkAndClearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex = 0u) noexcept;
	void setDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex = 0u) noexcept;
	bool gotAllExpectedDynamicInfo() const noexcept;
	std::pair<bool, std::chrono::milliseconds> getQueryDynamicInfoRetryTimer() noexcept;

	// Expected descriptor dynamic info query methods
	bool checkAndClearExpectedDescriptorDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	void setDescriptorDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	void clearAllExpectedDescriptorDynamicInfo() noexcept;
	bool gotAllExpectedDescriptorDynamicInfo() const noexcept;
	std::pair<bool, std::chrono::milliseconds> getQueryDescriptorDynamicInfoRetryTimer() noexcept;

	// Other getters/setters
	entity::Entity& getEntity() noexcept;
	bool shouldIgnoreCachedEntityModel() const noexcept;
	void setIgnoreCachedEntityModel() noexcept;
	EnumerationSteps getEnumerationSteps() const noexcept;
	void addEnumerationSteps(EnumerationSteps const steps) noexcept;
	void clearEnumerationSteps(EnumerationSteps const steps) noexcept;
	void setCompatibilityFlags(CompatibilityFlags const compatibilityFlags) noexcept;
	void setGetFatalEnumerationError() noexcept;
	void setSubscribedToUnsolicitedNotifications(bool const isSubscribed) noexcept;
	bool wasAdvertised() const noexcept;
	void setAdvertised(bool const wasAdvertised) noexcept;

	// Other usefull manipulation methods
	constexpr static bool isStreamRunningFlag(entity::StreamInfoFlags const flags) noexcept
	{
		return !utils::hasFlag(flags, entity::StreamInfoFlags::StreamingWait);
	}
	constexpr static void setStreamRunningFlag(entity::StreamInfoFlags& flags, bool const isRunning) noexcept
	{
		if (isRunning)
		{
			utils::clearFlag(flags, entity::StreamInfoFlags::StreamingWait);
		}
		else
		{
			utils::addFlag(flags, entity::StreamInfoFlags::StreamingWait);
		}
	}

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
	static void initNode(NodeType& node, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
	{
		node.descriptorType = descriptorType;
		node.descriptorIndex = descriptorIndex;
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
	LockInformation::SharedPointer _sharedLock{ nullptr };
	bool _ignoreCachedEntityModel{ false };
	std::uint16_t _queryMilanInfoRetryCount{ 0u };
	std::uint16_t _queryDescriptorRetryCount{ 0u };
	std::uint16_t _queryDynamicInfoRetryCount{ 0u };
	std::uint16_t _queryDescriptorDynamicInfoRetryCount{ 0u };
	EnumerationSteps _enumerationSteps{ EnumerationSteps::None };
	CompatibilityFlags _compatibilityFlags{ CompatibilityFlag::IEEE17221 }; // Entity is IEEE1722.1 compatible by default
	bool _gotFatalEnumerateError{ false }; // Have we got a fatal error during entity enumeration
	bool _isSubscribedToUnsolicitedNotifications{ false }; // Are we subscribed to unsolicited notifications
	bool _advertised{ false }; // Has the entity been advertised to the observers
	std::unordered_set<MilanInfoKey> _expectedMilanInfo{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DescriptorKey>> _expectedDescriptors{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DynamicInfoKey>> _expectedDynamicInfo{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DescriptorDynamicInfoKey>> _expectedDescriptorDynamicInfo{};
	std::unordered_map<entity::model::AvbInterfaceIndex, InterfaceLinkStatus> _avbInterfaceLinkStatus{}; // Link status for each AvbInterface (true = up or unknown, false = down)
	model::AcquireState _acquireState{ model::AcquireState::Undefined };
	UniqueIdentifier _owningControllerID{}; // EID of the controller currently owning (who acquired) this entity
	model::LockState _lockState{ model::LockState::Undefined };
	UniqueIdentifier _lockingControllerID{}; // EID of the controller currently locking (who locked) this entity
	// Milan specific information
	entity::model::MilanInfo _milanInfo{};
	// Entity variables
	entity::Entity _entity; // No NSMI, Entity has no default constructor but it has to be passed to the only constructor of this class anyway
	// Entity Model
	mutable model::EntityStaticTree _entityStaticTree{}; // Static part of the model as represented by the AVDECC protocol
	mutable model::EntityDynamicTree _entityDynamicTree{}; // Dynamic part of the model as represented by the AVDECC protocol
	mutable model::EntityNode _entityNode{}; // Model as represented by the ControlledEntity (tree of references to the model::EntityDescriptor and model::EntityDynamicInfo)
};

} // namespace controller

// Define bitfield enum traits for controller::ControlledEntityImpl::EnumerationSteps
template<>
struct utils::enum_traits<controller::ControlledEntityImpl::EnumerationSteps>
{
	static constexpr bool is_bitfield = true;
};

} // namespace avdecc
} // namespace la
