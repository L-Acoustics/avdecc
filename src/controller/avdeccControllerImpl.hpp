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
* @file avdeccControllerImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/controller/avdeccController.hpp"
#include "avdeccControlledEntityImpl.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace controller
{

#pragma message("TODO: Move code to .cpp file")
class EntityModelCache final
{
public:
	static EntityModelCache& getInstance() noexcept
	{
		static EntityModelCache s_instance{};
		return s_instance;
	}

	model::EntityStaticTree const* getCachedEntityStaticTree(UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) const noexcept
	{
		auto const entityModelIt = _modelCache.find(entityID);
		if (entityModelIt != _modelCache.end())
		{
			auto const& entityModel = entityModelIt->second;
			auto const modelIt = entityModel.find(configurationIndex);
			if (modelIt != entityModel.end())
			{
				return &modelIt->second;
			}
		}
		return nullptr;
	}

	void cacheEntityStaticTree(UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, model::EntityStaticTree const& staticTree) noexcept
	{
		auto& entityModel = _modelCache[entityID];

#pragma message("TODO: Replace with c++17 insert_or_assign")
		model::EntityStaticTree* cachedModel{ nullptr };
		auto modelIt = entityModel.find(configurationIndex);
		if (modelIt == entityModel.end())
		{
			cachedModel = &entityModel.insert(std::make_pair(configurationIndex, staticTree)).first->second;
		}
		else
		{
			modelIt->second = staticTree;
			cachedModel = &modelIt->second;
		}

		// TODO: Copy in the cache, but clear DescriptorDynamicInfo from it
		(void)cachedModel;
	}

private:
	using StaticEntityModel = std::unordered_map<entity::model::ConfigurationIndex, model::EntityStaticTree>;
	std::unordered_map<UniqueIdentifier, StaticEntityModel, la::avdecc::UniqueIdentifier::hash> _modelCache{};
};

class ControllerImpl final : public Controller, private entity::ControllerEntity::Delegate
{
public:
	ControllerImpl(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale);

private:
	using OnlineControlledEntity = std::shared_ptr<ControlledEntityImpl>;

	virtual ~ControllerImpl() override;

	/* ************************************************************ */
	/* Controller overrides                                         */
	/* ************************************************************ */
	virtual void destroy() noexcept override;

	virtual UniqueIdentifier getControllerEID() const noexcept override;

	/* Controller configuration */
	virtual void enableEntityAdvertising(std::uint32_t const availableDuration) override;
	virtual void disableEntityAdvertising() noexcept override;

	/* Enumeration and Control Protocol (AECP) */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept override;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept override;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept override;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept override;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept override;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept override;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept override;

	virtual ControlledEntityGuard getControlledEntity(UniqueIdentifier const entityID) const noexcept override;

	virtual void lock() noexcept override;
	virtual void unlock() noexcept override;

	/* ************************************************************ */
	/* Result handlers                                              */
	/* ************************************************************ */
	/* Enumeration and Control Protocol (AECP) handlers */
	void onGetMilanVersionResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept;
	void onRegisterUnsolicitedNotificationsResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept;
	void onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept;
	void onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept;
	void onAudioUnitDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AudioUnitDescriptor const& descriptor) noexcept;
	void onStreamInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onStreamOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onAvbInterfaceDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex, entity::model::AvbInterfaceDescriptor const& descriptor) noexcept;
	void onClockSourceDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex, entity::model::ClockSourceDescriptor const& descriptor) noexcept;
	void onMemoryObjectDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::MemoryObjectDescriptor const& descriptor) noexcept;
	void onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept;
	void onStringsDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::StringsDescriptor const& descriptor) noexcept;
	void onStreamPortInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept;
	void onStreamPortOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept;
	void onAudioClusterDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, entity::model::AudioClusterDescriptor const& descriptor) noexcept;
	void onAudioMapDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, entity::model::AudioMapDescriptor const& descriptor) noexcept;
	void onClockDomainDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainDescriptor const& descriptor) noexcept;
	void onGetStreamInputInfoResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamOutputInfoResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamPortInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamPortOutputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetAvbInfoResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	/* Connection Management Protocol (ACMP) handlers */
	void onConnectStreamResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onDisconnectStreamResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onGetTalkerStreamStateResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetListenerStreamStateResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetTalkerStreamConnectionResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept;

	/* ************************************************************ */
	/* entity::ControllerEntity::Delegate overrides                 */
	/* ************************************************************ */
	/* Global notifications */
	virtual void onTransportError(entity::ControllerEntity const* const /*controller*/) noexcept override;
	/* Discovery Protocol (ADP) delegate */
	virtual void onEntityOnline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::Entity const& /*entity*/) noexcept override;
	virtual void onEntityUpdate(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::Entity const& /*entity*/) noexcept override; // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	virtual void onEntityOffline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/) noexcept override;
	/* Connection Management Protocol sniffed messages (ACMP) */
	virtual void onControllerConnectResponseSniffed(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onControllerDisconnectResponseSniffed(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onListenerConnectResponseSniffed(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onListenerDisconnectResponseSniffed(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onGetListenerStreamStateResponseSniffed(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
	virtual void onEntityAcquired(entity::ControllerEntity const* const controller, UniqueIdentifier const acquiredEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onEntityReleased(entity::ControllerEntity const* const controller, UniqueIdentifier const releasedEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onConfigurationChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept override;
	virtual void onStreamInputFormatChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamOutputFormatChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamPortInputAudioMappingsChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamPortOutputAudioMappingsChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamInputInfoChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept override;
	virtual void onStreamOutputInfoChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept override;
	virtual void onEntityNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept override;
	virtual void onEntityGroupNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept override;
	virtual void onConfigurationNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept override;
	virtual void onStreamInputNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onStreamOutputNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onAudioUnitSamplingRateChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept override;
	virtual void onClockSourceChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept override;
	virtual void onStreamInputStarted(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStarted(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamInputStopped(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStopped(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onAvbInfoChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept override;

	/* ************************************************************ */
	/* Private methods used to update AEM and notify observers      */
	/* ************************************************************ */
	void updateEntity(ControlledEntityImpl& controlledEntity, entity::Entity const& entity, bool const alsoUpdateAvbInfo = true) const noexcept;
	void updateAcquiredState(ControlledEntityImpl& controlledEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, bool const undefined = false) const noexcept;
	void updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const noexcept;
	void updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const noexcept;
	void updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const noexcept;
	void updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const noexcept;
	void updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const noexcept;
	void updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const noexcept;
	void updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const noexcept;
	void updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const noexcept;
	void updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const noexcept;
	void updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const noexcept;
	void updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const noexcept;
	void updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const noexcept;
	void updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept;
	void updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept;
	void updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, bool const alsoUpdateEntity = true) const noexcept;
	void updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;

	/* ************************************************************ */
	/* Private enums                                                */
	/* ************************************************************ */
	enum class FailureAction
	{
		Ignore,
		Retry,
		Fatal,
	};

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	void getMilanVersion(ControlledEntityImpl* const entity) noexcept;
	void registerUnsol(ControlledEntityImpl* const entity) noexcept;
	void getStaticModel(ControlledEntityImpl* const entity) noexcept;
	void getDynamicInfo(ControlledEntityImpl* const entity) noexcept;
	void getDescriptorDynamicInfo(ControlledEntityImpl* const entity) noexcept;
	void checkEnumerationSteps(ControlledEntityImpl* const entity) noexcept;
	FailureAction getFailureAction(entity::ControllerEntity::AemCommandStatus const status) const noexcept;
	FailureAction getFailureAction(entity::ControllerEntity::ControlStatus const status) const noexcept;
	void rescheduleQuery(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	void rescheduleQuery(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	void rescheduleQuery(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex) const noexcept;
	void rescheduleQuery(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	bool processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	bool processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	bool processFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex = 0u) const noexcept;
	bool processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	void handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept;
	void handleTalkerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept;
	void clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex) const noexcept;
	void addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept;
	void delTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept;
	constexpr Controller& getSelf() const noexcept
	{
		return *const_cast<Controller*>(static_cast<Controller const*>(this));
	}
	inline OnlineControlledEntity getControlledEntityImpl(UniqueIdentifier const entityID) const noexcept
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt != _controlledEntities.end())
		{
			return entityIt->second;
		}
		return OnlineControlledEntity{};
	}

	/* ************************************************************ */
	/* Private members                                              */
	/* ************************************************************ */
	std::unordered_map<UniqueIdentifier, OnlineControlledEntity, UniqueIdentifier::hash> _controlledEntities;
	mutable std::mutex _lock{}; // A mutex to protect _controlledEntities
	EndStation::UniquePointer _endStation{ nullptr, nullptr };
	entity::ControllerEntity* _controller{ nullptr };
	std::string _preferedLocale{ "en-US" };
};

} // namespace controller

} // namespace avdecc
} // namespace la
