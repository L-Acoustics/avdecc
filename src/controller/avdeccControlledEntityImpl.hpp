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
* @file avdeccControlledEntityImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "treeModelAccessStrategy.hpp"

#include <la/avdecc/internals/entityModelTree.hpp>

#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <functional>
#include <chrono>
#include <mutex>
#include <utility>
#include <thread>
#include <optional>

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

		std::recursive_mutex _lock{};
		std::uint32_t _lockedCount{ 0u };
		std::thread::id _lockingThreadID{};

		void lock() noexcept
		{
			_lock.lock();
			if (_lockedCount == 0)
			{
				_lockingThreadID = std::this_thread::get_id();
			}
			++_lockedCount;
		}

		void unlock() noexcept
		{
			AVDECC_ASSERT(isSelfLocked(), "unlock should not be called when current thread is not the lock holder");

			--_lockedCount;
			if (_lockedCount == 0)
			{
				_lockingThreadID = {};
			}
			_lock.unlock();
		}

		void lockAll(std::uint32_t const lockedCount) noexcept
		{
			for (auto count = 0u; count < lockedCount; ++count)
			{
				lock();
			}
		}

		std::uint32_t unlockAll() noexcept
		{
			AVDECC_ASSERT(isSelfLocked(), "unlockAll should not be called when current thread is not the lock holder");

			auto result = 0u;
			[[maybe_unused]] auto const previousLockedCount = _lockedCount;
			while (isSelfLocked())
			{
				unlock();
				++result;
			}

			AVDECC_ASSERT(previousLockedCount == result, "lockedCount does not match the number of unlockings");
			return result;
		}

		bool isSelfLocked() const noexcept
		{
			return _lockingThreadID == std::this_thread::get_id();
		}
	};

	enum class EnumerationStep : std::uint16_t
	{
		GetMilanInfo = 1u << 0,
		RegisterUnsol = 1u << 1,
		GetStaticModel = 1u << 2,
		GetDescriptorDynamicInfo = 1u << 3, /** DescriptorDynamicInfoType */
		GetDynamicInfo = 1u << 4, /** DynamicInfoType */
	};
	using EnumerationSteps = utils::EnumBitfield<EnumerationStep>;

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
		GetEntityCounters, // getEntityCounters (GET_COUNTERS)
		GetAvbInterfaceCounters, // getAvbInterfaceCounters (GET_COUNTERS)
		GetClockDomainCounters, // getClockDomainCounters (GET_COUNTERS)
		GetStreamInputCounters, // getStreamInputCounters (GET_COUNTERS)
		GetStreamOutputCounters, // getStreamOutputCounters (GET_COUNTERS)
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
		InputJackName, // JACK_INPUT.object_name -> GET_NAME (7.4.18)
		OutputJackName, // JACK_OUTPUT.object_name -> GET_NAME (7.4.18)
		AvbInterfaceName, // AVB_INTERFACE.object_name -> GET_NAME (7.4.18)
		ClockSourceName, // CLOCK_SOURCE.object_name -> GET_NAME (7.4.18)
		MemoryObjectName, // MEMORY_OBJECT.object_name -> GET_NAME (7.4.18)
		MemoryObjectLength, // MEMORY_OBJECT.length -> GET_MEMORY_OBJECT_LENGTH (7.4.73)
		AudioClusterName, // AUDIO_CLUSTER.object_name -> GET_NAME (7.4.18)
		ControlName, // CONTROL.object_name -> GET_NAME (7.4.18)
		ControlValues, // CONTROL.value_details -> GET_CONTROL (7.4.26)
		ClockDomainName, // CLOCK_DOMAIN.object_name -> GET_NAME (7.4.18)
		ClockDomainSourceIndex, // CLOCK_DOMAIN.clock_source_index -> GET_CLOCK_SOURCE (7.4.24)
		TimingName, // TIMING.object_name -> GET_NAME (7.4.18)
		PtpInstanceName, // PTP_INSTANCE.object_name -> GET_NAME (7.4.18)
		PtpPortName, // PTP_PORT.object_name -> GET_NAME (7.4.18)
	};

	using MilanInfoKey = std::underlying_type_t<MilanInfoType>;
	using DescriptorKey = std::uint32_t;
	static_assert(sizeof(DescriptorKey) >= sizeof(entity::model::DescriptorType) + sizeof(entity::model::DescriptorIndex), "DescriptorKey size must be greater or equal to DescriptorType + DescriptorIndex");
	using DynamicInfoKey = std::uint64_t;
	static_assert(sizeof(DynamicInfoKey) >= sizeof(DynamicInfoType) + sizeof(entity::model::DescriptorIndex) + sizeof(std::uint16_t), "DynamicInfoKey size must be greater or equal to DynamicInfoType + DescriptorIndex + std::uint16_t");
	using DescriptorDynamicInfoKey = std::uint64_t;
	static_assert(sizeof(DescriptorDynamicInfoKey) >= sizeof(DescriptorDynamicInfoType) + sizeof(entity::model::DescriptorIndex), "DescriptorDynamicInfoKey size must be greater or equal to DescriptorDynamicInfoType + DescriptorIndex");

	/** Constructor */
	ControlledEntityImpl(la::avdecc::entity::Entity const& entity, LockInformation::SharedPointer const& sharedLock, bool const isVirtual) noexcept;

	// ControlledEntity overrides
	// Getters
	virtual bool isVirtual() const noexcept override;
	virtual CompatibilityFlags getCompatibilityFlags() const noexcept override;
	virtual bool isMilanRedundant() const noexcept override;
	virtual bool gotFatalEnumerationError() const noexcept override;
	virtual bool isSubscribedToUnsolicitedNotifications() const noexcept override;
	virtual bool areUnsolicitedNotificationsSupported() const noexcept override;
	virtual bool isAcquired() const noexcept override;
	virtual bool isAcquireCommandInProgress() const noexcept override;
	virtual bool isAcquiredByOther() const noexcept override;
	virtual bool isLocked() const noexcept override;
	virtual bool isLockCommandInProgress() const noexcept override;
	virtual bool isLockedByOther() const noexcept override;
	virtual bool isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual bool isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual InterfaceLinkStatus getAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex) const noexcept override;
	virtual model::AcquireState getAcquireState() const noexcept override;
	virtual UniqueIdentifier getOwningControllerID() const noexcept override;
	virtual model::LockState getLockState() const noexcept override;
	virtual UniqueIdentifier getLockingControllerID() const noexcept override;
	virtual entity::Entity const& getEntity() const noexcept override;
	virtual std::optional<entity::model::MilanInfo> getMilanInfo() const noexcept override;
	virtual std::optional<entity::model::ControlIndex> getIdentifyControlIndex() const noexcept override;
	virtual bool isEntityModelValidForCaching() const noexcept override;
	virtual bool isIdentifying() const noexcept override;
	virtual bool hasAnyConfiguration() const noexcept override;
	virtual entity::model::ConfigurationIndex getCurrentConfigurationIndex() const override;

	// Const Node getters
	virtual model::EntityNode const& getEntityNode() const override;
	virtual model::ConfigurationNode const& getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex) const override;
	virtual model::ConfigurationNode const& getCurrentConfigurationNode() const override;
	virtual model::AudioUnitNode const& getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const override;
	virtual model::StreamInputNode const& getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
	virtual model::StreamOutputNode const& getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const override;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::RedundantStreamNode const& getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const override;
	virtual model::RedundantStreamNode const& getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const override;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::JackInputNode const& getJackInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) const override;
	virtual model::JackOutputNode const& getJackOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) const override;
	virtual model::AvbInterfaceNode const& getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const override;
	virtual model::ClockSourceNode const& getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const override;
	virtual model::StreamPortNode const& getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const override;
	virtual model::StreamPortNode const& getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const override;
	virtual model::AudioClusterNode const& getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const override;
	//virtual model::AudioMapNode const& getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const override;
	virtual model::ControlNode const& getControlNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex) const override;
	virtual model::ClockDomainNode const& getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const override;
	virtual model::TimingNode const& getTimingNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex) const override;
	virtual model::PtpInstanceNode const& getPtpInstanceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex) const override;
	virtual model::PtpPortNode const& getPtpPortNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex) const override;

	virtual model::LocaleNode const* findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& locale) const override; // Throws Exception::NotSupported if EM not supported by the Entity // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::LocalizedStringReference const& stringReference) const noexcept override;
	virtual entity::model::AvdeccFixedString const& getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const& stringReference) const noexcept override; // Get localized string or empty string if not found // Throws Exception::InvalidConfigurationIndex if configurationIndex do not exist

	// Visitor method
	virtual void accept(model::EntityModelVisitor* const visitor, bool const visitAllConfigurations = false) const noexcept override;

	virtual void lock() noexcept override;
	virtual void unlock() noexcept override;

	virtual entity::model::StreamInputConnectionInfo const& getSinkConnectionInformation(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist
	virtual entity::model::AudioMappings const& getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	virtual entity::model::AudioMappings getStreamPortInputNonRedundantAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	virtual entity::model::AudioMappings const& getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	virtual entity::model::AudioMappings getStreamPortOutputNonRedundantAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamPortIndex do not exist
	virtual std::map<entity::model::StreamPortIndex, entity::model::AudioMappings> getStreamPortInputInvalidAudioMappingsForStreamFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const override; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist

	/** Get connections information about a talker's stream */
	virtual entity::model::StreamConnections const& getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const override; // Throws Exception::InvalidDescriptorIndex if streamIndex do not exist

	// Statistics
	virtual std::uint64_t getAecpRetryCounter() const noexcept override;
	virtual std::uint64_t getAecpTimeoutCounter() const noexcept override;
	virtual std::uint64_t getAecpUnexpectedResponseCounter() const noexcept override;
	virtual std::chrono::milliseconds const& getAecpResponseAverageTime() const noexcept override;
	virtual std::uint64_t getAemAecpUnsolicitedCounter() const noexcept override;
	virtual std::uint64_t getAemAecpUnsolicitedLossCounter() const noexcept override;
	virtual std::chrono::milliseconds const& getEnumerationTime() const noexcept override;

	// Diagnostics
	virtual Diagnostics const& getDiagnostics() const noexcept override;

	TreeModelAccessStrategy& getModelAccessStrategy() noexcept;

	// Non-const Node getters. Behavior in case of error is dictated by the passed NotFoundBehavior
	model::EntityNode* getEntityNode(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	std::optional<entity::model::ConfigurationIndex> getCurrentConfigurationIndex(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	model::ConfigurationNode* getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	model::ConfigurationNode* getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	entity::model::EntityCounters* getEntityCounters(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	entity::model::AvbInterfaceCounters* getAvbInterfaceCounters(entity::model::AvbInterfaceIndex const avbInterfaceIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	entity::model::ClockDomainCounters* getClockDomainCounters(entity::model::ClockDomainIndex const clockDomainIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	entity::model::StreamInputCounters* getStreamInputCounters(entity::model::StreamIndex const streamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	entity::model::StreamOutputCounters* getStreamOutputCounters(entity::model::StreamIndex const streamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);

	// Setters of the DescriptorDynamic info
	void setEntityName(entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void setEntityGroupName(entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void setCurrentConfiguration(entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void setConfigurationName(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	template<typename TreeModelAccessPointer, typename DescriptorIndexType>
	void setObjectName(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const index, TreeModelAccessPointer TreeModelAccessStrategy::*Pointer, entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
	{
		auto* const dynamicModel = ((*_treeModelAccess).*Pointer)(configurationIndex, index, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->objectName = name;
		}
	}
	void setSamplingRate(entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	entity::model::StreamInputConnectionInfo setStreamInputConnectionInformation(entity::model::StreamIndex const streamIndex, entity::model::StreamInputConnectionInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void clearStreamOutputConnections(entity::model::StreamIndex const streamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	bool addStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior); // Returns true if effectively added
	bool delStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior); // Returns true if effectively removed
	entity::model::AvbInterfaceInfo setAvbInterfaceInfo(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInterfaceInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior); // Returns previous AvbInterfaceInfo
	entity::model::AsPath setAsPath(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior); // Returns previous AsPath
	void setSelectedLocaleStringsIndexesRange(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex, entity::model::StringsIndex const countIndexes, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void clearStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void addStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void removeStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void clearStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void addStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void removeStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void setClockSource(entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void setControlValues(entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);
	void setMemoryObjectLength(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior);

	// Setters of the global state
	void setEntity(entity::Entity const& entity) noexcept;
	InterfaceLinkStatus setAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex, InterfaceLinkStatus const linkStatus) noexcept; // Returns previous link status
	void setAcquireState(model::AcquireState const state) noexcept;
	void setOwningController(UniqueIdentifier const controllerID) noexcept;
	void setLockState(model::LockState const state) noexcept;
	void setLockingController(UniqueIdentifier const controllerID) noexcept;
	void setMilanInfo(entity::model::MilanInfo const& info) noexcept;

	// Setters of the Statistics
	void setAecpRetryCounter(std::uint64_t const value) noexcept;
	void setAecpTimeoutCounter(std::uint64_t const value) noexcept;
	void setAecpUnexpectedResponseCounter(std::uint64_t const value) noexcept;
	void setAecpResponseAverageTime(std::chrono::milliseconds const& value) noexcept;
	void setAemAecpUnsolicitedCounter(std::uint64_t const value) noexcept;
	void setAemAecpUnsolicitedLossCounter(std::uint64_t const value) noexcept;
	void setEnumerationTime(std::chrono::milliseconds const& value) noexcept;

	// Setters of the Diagnostics
	void setDiagnostics(Diagnostics const& diags) noexcept;

	// Setters of the Model from AEM Descriptors (including DescriptorDynamic info)
	bool setCachedEntityNode(model::EntityNode&& cachedNode, entity::model::EntityDescriptor const& descriptor, bool const forAllConfiguration) noexcept; // Returns true if the cached EntityNode is accepted (and set) for this entity
	void setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept;
	void setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) noexcept;
	void setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept;
	void setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept;
	void setJackInputDescriptor(entity::model::JackDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) noexcept;
	void setJackOutputDescriptor(entity::model::JackDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) noexcept;
	void setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex) noexcept;
	void setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex) noexcept;
	void setMemoryObjectDescriptor(entity::model::MemoryObjectDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex) noexcept;
	void setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex) noexcept;
	void setStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex) noexcept;
	void setLocalizedStrings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const relativeStringsIndex, entity::model::AvdeccFixedStrings const& strings) noexcept;
	void setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept;
	void setStreamPortOutputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept;
	void setAudioClusterDescriptor(entity::model::AudioClusterDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) noexcept;
	void setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) noexcept;
	void setControlDescriptor(entity::model::ControlDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex) noexcept;
	void setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) noexcept;
	void setTimingDescriptor(entity::model::TimingDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex) noexcept;
	void setPtpInstanceDescriptor(entity::model::PtpInstanceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex) noexcept;
	void setPtpPortDescriptor(entity::model::PtpPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex) noexcept;

	// Setters of statistics
	std::uint64_t incrementAecpRetryCounter() noexcept;
	std::uint64_t incrementAecpTimeoutCounter() noexcept;
	std::uint64_t incrementAecpUnexpectedResponseCounter() noexcept;
	std::chrono::milliseconds const& updateAecpResponseTimeAverage(std::chrono::milliseconds const& responseTime) noexcept;
	std::uint64_t incrementAemAecpUnsolicitedCounter() noexcept;
	std::uint64_t incrementAemAecpUnsolicitedLossCounter() noexcept;
	void setStartEnumerationTime(std::chrono::time_point<std::chrono::steady_clock>&& startTime) noexcept;
	void setEndEnumerationTime(std::chrono::time_point<std::chrono::steady_clock>&& endTime) noexcept;

	// Expected RegisterUnsol query methods
	bool checkAndClearExpectedRegisterUnsol() noexcept;
	void setRegisterUnsolExpected() noexcept;
	bool gotExpectedRegisterUnsol() const noexcept;
	std::pair<bool, std::chrono::milliseconds> getRegisterUnsolRetryTimer() noexcept;

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
	void setIdentifyControlIndex(entity::model::ControlIndex const identifyControlIndex) noexcept;
	bool shouldIgnoreCachedEntityModel() const noexcept;
	void setIgnoreCachedEntityModel() noexcept;
	EnumerationSteps getEnumerationSteps() const noexcept;
	void setEnumerationSteps(EnumerationSteps const steps) noexcept;
	void addEnumerationStep(EnumerationStep const step) noexcept;
	void clearEnumerationStep(EnumerationStep const step) noexcept;
	void setCompatibilityFlags(CompatibilityFlags const compatibilityFlags) noexcept;
	void setMilanRedundant(bool const isMilanRedundant) noexcept;
	void setGetFatalEnumerationError() noexcept;
	void setSubscribedToUnsolicitedNotifications(bool const isSubscribed) noexcept;
	void setUnsolicitedNotificationsSupported(bool const isSupported) noexcept;
	bool wasAdvertised() const noexcept;
	void setAdvertised(bool const wasAdvertised) noexcept;
	bool isRedundantPrimaryStreamInput(entity::model::StreamIndex const streamIndex) const noexcept; // True for a Redundant Primary Stream (false for Secondary and non-redundant streams)
	bool isRedundantPrimaryStreamOutput(entity::model::StreamIndex const streamIndex) const noexcept; // True for a Redundant Primary Stream (false for Secondary and non-redundant streams)
	bool isRedundantSecondaryStreamInput(entity::model::StreamIndex const streamIndex) const noexcept; // True for a Redundant Secondary Stream (false for Primary and non-redundant streams)
	bool isRedundantSecondaryStreamOutput(entity::model::StreamIndex const streamIndex) const noexcept; // True for a Redundant Secondary Stream (false for Primary and non-redundant streams)
	Diagnostics& getDiagnostics() noexcept;
	bool hasLostUnsolicitedNotification(protocol::AecpSequenceID const sequenceID) noexcept;
	entity::model::EntityTree const& getEntityModelTree() const noexcept;
	void buildEntityModelGraph(entity::model::EntityTree const& entityTree) noexcept;

	// Static methods
	static std::string dynamicInfoTypeToString(DynamicInfoType const dynamicInfoType) noexcept;
	static std::string descriptorDynamicInfoTypeToString(DescriptorDynamicInfoType const descriptorDynamicInfoType) noexcept;

	// Controller restricted methods
	void onEntityModelEnumerated() noexcept; // To be called when the entity model has been fully retrieved
	void onEntityFullyLoaded() noexcept; // To be called when the entity has been fully loaded and is ready to be shared

	// Compiler auto-generated methods
	ControlledEntityImpl(ControlledEntityImpl&&) = delete;
	ControlledEntityImpl(ControlledEntityImpl const&) = delete;
	ControlledEntityImpl& operator=(ControlledEntityImpl const&) = delete;
	ControlledEntityImpl& operator=(ControlledEntityImpl&&) = delete;

protected:
	using RedundantStreamCategory = std::unordered_set<entity::model::StreamIndex>;

private:
	// Private methods
	void switchToCachedTreeModelAccessStrategy() noexcept;
	bool isEntityModelComplete(model::EntityNode const& entityNode, std::uint16_t const configurationsCount) const noexcept;
	void buildVirtualNodes(model::ConfigurationNode& configNode) noexcept;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	void buildRedundancyNodes(model::ConfigurationNode& configNode) noexcept;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Private variables
	LockInformation::SharedPointer _sharedLock{ nullptr };
	bool const _isVirtual{ false };
	bool _ignoreCachedEntityModel{ false };
	std::optional<entity::model::ControlIndex> _identifyControlIndex{ std::nullopt };
	std::uint16_t _registerUnsolRetryCount{ 0u };
	std::uint16_t _queryMilanInfoRetryCount{ 0u };
	std::uint16_t _queryDescriptorRetryCount{ 0u };
	std::uint16_t _queryDynamicInfoRetryCount{ 0u };
	std::uint16_t _queryDescriptorDynamicInfoRetryCount{ 0u };
	EnumerationSteps _enumerationSteps{};
	CompatibilityFlags _compatibilityFlags{ CompatibilityFlag::IEEE17221 }; // Entity is IEEE1722.1 compatible by default
	bool _isMilanRedundant{ false }; // Current configuration has at least one redundant stream
	bool _gotFatalEnumerateError{ false }; // Have we got a fatal error during entity enumeration
	bool _isSubscribedToUnsolicitedNotifications{ false }; // Are we subscribed to unsolicited notifications
	bool _areUnsolicitedNotificationsSupported{ false }; // Are unsolicited notifications supported
	bool _advertised{ false }; // Has the entity been advertised to the observers
	bool _expectedRegisterUnsol{ false };
	std::unordered_set<MilanInfoKey> _expectedMilanInfo{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DescriptorKey>> _expectedDescriptors{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DynamicInfoKey>> _expectedDynamicInfo{};
	std::unordered_map<entity::model::ConfigurationIndex, std::unordered_set<DescriptorDynamicInfoKey>> _expectedDescriptorDynamicInfo{};
	std::unordered_map<entity::model::AvbInterfaceIndex, InterfaceLinkStatus> _avbInterfaceLinkStatus{}; // Link status for each AvbInterface (true = up or unknown, false = down)
	model::AcquireState _acquireState{ model::AcquireState::Undefined };
	UniqueIdentifier _owningControllerID{}; // EID of the controller currently owning (who acquired) this entity
	model::LockState _lockState{ model::LockState::Undefined };
	UniqueIdentifier _lockingControllerID{}; // EID of the controller currently locking (who locked) this entity
	std::optional<protocol::AecpSequenceID> _expectedSequenceID{ std::nullopt };
	// Milan specific information
	std::optional<entity::model::MilanInfo> _milanInfo{ std::nullopt };
	// Entity variables
	entity::Entity _entity; // No NSMI, Entity has no default constructor but it has to be passed to the only constructor of this class anyway
	// Entity Model
	//entity::model::EntityTree _entityTree{}; // Tree of the model as represented by the AVDECC protocol
	model::EntityNode _entityNode{}; // Model as represented by the ControlledEntity (tree of references to the model::EntityStaticTree and model::EntityDynamicTree)
	// Entity Model Tree Access Strategy
	friend class TreeModelAccessTraverseStrategy;
	friend class TreeModelAccessCacheStrategy;
	TreeModelAccessStrategy::UniquePointer _treeModelAccess{ nullptr };
	// Cached Information
	mutable std::optional<entity::model::EntityTree> _entityTree{};
	RedundantStreamCategory _redundantPrimaryStreamInputs{}; // Cached indexes of all Redundant Primary Streams (a non-redundant stream won't be listed here)
	RedundantStreamCategory _redundantPrimaryStreamOutputs{}; // Cached indexes of all Redundant Primary Streams (a non-redundant stream won't be listed here)
	RedundantStreamCategory _redundantSecondaryStreamInputs{}; // Cached indexes of all Redundant Secondary Streams
	RedundantStreamCategory _redundantSecondaryStreamOutputs{}; // Cached indexes of all Redundant Secondary Streams
	// Statistics
	std::uint64_t _aecpRetryCounter{ 0ull };
	std::uint64_t _aecpTimeoutCounter{ 0ull };
	std::uint64_t _aecpUnexpectedResponseCounter{ 0ull };
	std::uint64_t _aecpResponsesCount{ 0ull }; // Intermediate variable used by _aecpResponseAverageTime
	std::chrono::milliseconds _aecpResponseTimeSum{}; // Intermediate variable used by _aecpResponseAverageTime
	std::chrono::milliseconds _aecpResponseAverageTime{};
	std::uint64_t _aemAecpUnsolicitedCounter{ 0ull };
	std::uint64_t _aemAecpUnsolicitedLossCounter{ 0ull };
	std::chrono::time_point<std::chrono::steady_clock> _enumerationStartTime{}; // Intermediate variable used by _enumerationTime
	std::chrono::milliseconds _enumerationTime{};
	// Diagnostics
	Diagnostics _diagnostics{};
};

} // namespace controller
} // namespace avdecc
} // namespace la
