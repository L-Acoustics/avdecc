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
* @file avdeccControllerImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/controller/avdeccController.hpp"
#include "la/avdecc/memoryBuffer.hpp"
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include <la/avdecc/internals/jsonSerialization.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON

#include "avdeccControlledEntityImpl.hpp"
#include "avdeccControllerProxy.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <chrono>
#include <optional>
#include <deque>
#include <tuple>
#include <set>

namespace la
{
namespace avdecc
{
namespace controller
{
class ExclusiveAccessTokenImpl;

/* ************************************************************************** */
/* ControllerImpl class definition                                            */
/* ************************************************************************** */
class ControllerImpl final : public Controller, private entity::controller::DefaultedDelegate
{
public:
	using SharedControlledEntityImpl = std::shared_ptr<ControlledEntityImpl>;

	ControllerImpl(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree, std::optional<std::string> const& executorName, entity::controller::Interface const* const virtualEntityInterface);

	void unregisterExclusiveAccessToken(la::avdecc::UniqueIdentifier const entityID, ExclusiveAccessTokenImpl* const token) const noexcept;
	static std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, std::vector<SharedControlledEntity>> deserializeControlledEntitiesFromJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError) noexcept;
	static std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, SharedControlledEntity> deserializeControlledEntityFromJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags) noexcept;

#ifndef la_avdecc_controller_static_STATICS /* Keep everything public when compiling the static library so unit tests can access all methods */
private:
#endif // !la_avdecc_controller_static_STATICS
	virtual ~ControllerImpl() override;

	/* ************************************************************ */
	/* Controller overrides                                         */
	/* ************************************************************ */
	virtual void destroy() noexcept override;

	virtual UniqueIdentifier getControllerEID() const noexcept override;

	/* Controller configuration */
	virtual void enableEntityAdvertising(std::uint32_t const availableDuration, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) override;
	virtual void disableEntityAdvertising(std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) noexcept override;
	virtual bool discoverRemoteEntities() const noexcept override;
	virtual bool discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override;
	virtual void setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) noexcept override;
	virtual void enableEntityModelCache() noexcept override;
	virtual void disableEntityModelCache() noexcept override;
	virtual void enableFullStaticEntityModelEnumeration() noexcept override;
	virtual void disableFullStaticEntityModelEnumeration() noexcept override;

	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept override;
	virtual void unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept override;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void setStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept override;
	virtual void setStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& name, SetAudioUnitNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void setJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& name, SetJackInputNameHandler const& handler) const noexcept override;
	virtual void setJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& name, SetJackOutputNameHandler const& handler) const noexcept override;
	virtual void setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& name, SetAvbInterfaceNameHandler const& handler) const noexcept override;
	virtual void setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& name, SetClockSourceNameHandler const& handler) const noexcept override;
	virtual void setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& name, SetMemoryObjectNameHandler const& handler) const noexcept override;
	virtual void setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& name, SetAudioClusterNameHandler const& handler) const noexcept override;
	virtual void setControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& name, SetControlNameHandler const& handler) const noexcept override;
	virtual void setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& name, SetClockDomainNameHandler const& handler) const noexcept override;
	virtual void setAssociationID(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationIDHandler const& handler) const noexcept override;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept override;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept override;
	virtual void setControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept override;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept override;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept override;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept override;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept override;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept override;
	virtual void rebootToFirmware(UniqueIdentifier const targetEntityID, entity::model::MemoryObjectIndex const memoryObjectIndex, RebootHandler const& handler) const noexcept override;
	virtual void startStoreMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept override;
	virtual void startStoreAndRebootMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept override;
	virtual void startReadMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept override;
	virtual void startEraseMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept override;
	virtual void startUploadMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, std::uint64_t const dataLength, StartMemoryObjectOperationHandler const& handler) const noexcept override;
	virtual void abortOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept override;
	virtual void setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept override;
	virtual void identifyEntity(UniqueIdentifier const targetEntityID, std::chrono::milliseconds const duration, IdentifyEntityHandler const& handler) const noexcept override;

	/* Enumeration and Control Protocol (AECP) AA */
	virtual void readDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, std::uint64_t const length, ReadDeviceMemoryProgressHandler const& progressHandler, ReadDeviceMemoryCompletionHandler const& completionHandler) const noexcept override;
	virtual void writeDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, DeviceMemoryBuffer memoryBuffer, WriteDeviceMemoryProgressHandler const& progressHandler, WriteDeviceMemoryCompletionHandler const& completionHandler) const noexcept override;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept override;

	virtual ControlledEntityGuard getControlledEntityGuard(UniqueIdentifier const entityID) const noexcept override;

	virtual void requestExclusiveAccess(UniqueIdentifier const entityID, ExclusiveAccessToken::AccessType const type, RequestExclusiveAccessResultHandler&& handler) const noexcept override;

	virtual void lock() noexcept override;
	virtual void unlock() noexcept override;

	/* Model serialization methods */
	virtual std::tuple<avdecc::jsonSerializer::SerializationError, std::string> serializeAllControlledEntitiesAsJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, std::string const& dumpSource, bool const continueOnError) const noexcept override;
	virtual std::tuple<avdecc::jsonSerializer::SerializationError, std::string> serializeControlledEntityAsJson(UniqueIdentifier const entityID, std::string const& filePath, entity::model::jsonSerializer::Flags const flags, std::string const& dumpSource) const noexcept override;

	/* Model deserialization methods */
	virtual std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> loadVirtualEntitiesFromJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError) noexcept override;
	virtual std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> loadVirtualEntityFromJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags) noexcept override;
	virtual std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> loadEntityModelFile(std::string const& filePath) noexcept override;

	/* Other helpful methods */
	virtual bool refreshEntity(UniqueIdentifier const entityID) noexcept override;
	virtual bool unloadVirtualEntity(UniqueIdentifier const entityID) noexcept override;


	/* ************************************************************ */
	/* Result handlers                                              */
	/* ************************************************************ */
	/* Enumeration and Control Protocol (AECP) handlers */
	void onGetMilanInfoResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::MvuCommandStatus const status, entity::model::MilanInfo const& info) noexcept;
	void onRegisterUnsolicitedNotificationsResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept;
	void onUnregisterUnsolicitedNotificationsResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept;
	void onEntityDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept;
	void onConfigurationDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept;
	void onAudioUnitDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AudioUnitDescriptor const& descriptor) noexcept;
	void onStreamInputDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onStreamOutputDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onJackInputDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::JackDescriptor const& descriptor) noexcept;
	void onJackOutputDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::JackDescriptor const& descriptor) noexcept;
	void onAvbInterfaceDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex, entity::model::AvbInterfaceDescriptor const& descriptor) noexcept;
	void onClockSourceDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex, entity::model::ClockSourceDescriptor const& descriptor) noexcept;
	void onMemoryObjectDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::MemoryObjectDescriptor const& descriptor) noexcept;
	void onLocaleDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept;
	void onStringsDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor) noexcept;
	void onStreamPortInputDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept;
	void onStreamPortOutputDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept;
	void onAudioClusterDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, entity::model::AudioClusterDescriptor const& descriptor) noexcept;
	void onAudioMapDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, entity::model::AudioMapDescriptor const& descriptor) noexcept;
	void onControlDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::ControlDescriptor const& descriptor) noexcept;
	void onClockDomainDescriptorResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainDescriptor const& descriptor) noexcept;
	void onGetStreamInputInfoResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamOutputInfoResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetAcquiredStateResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity) noexcept;
	void onGetLockedStateResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const lockingEntity) noexcept;
	void onGetStreamPortInputAudioMapResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamPortOutputAudioMapResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetAvbInfoResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetAsPathResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetEntityCountersResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept;
	void onGetAvbInterfaceCountersResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetClockDomainCountersResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamInputCountersResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamOutputCountersResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onConfigurationNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept;
	void onAudioUnitNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) noexcept;
	void onAudioUnitSamplingRateResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onInputStreamNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) noexcept;
	void onInputStreamFormatResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onOutputStreamNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) noexcept;
	void onOutputStreamFormatResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onInputJackNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName) noexcept;
	void onOutputJackNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName) noexcept;
	void onAvbInterfaceNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) noexcept;
	void onClockSourceNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) noexcept;
	void onMemoryObjectNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) noexcept;
	void onMemoryObjectLengthResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept;
	void onAudioClusterNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) noexcept;
	void onControlNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName) noexcept;
	void onControlValuesResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onClockDomainNameResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) noexcept;
	void onClockDomainSourceIndexResult(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::ConfigurationIndex const configurationIndex) noexcept;

	/* Connection Management Protocol (ACMP) handlers */
	void onConnectStreamResult(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onDisconnectStreamResult(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onGetTalkerStreamStateResult(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetListenerStreamStateResult(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetTalkerStreamConnectionResult(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept;

	/* ************************************************************ */
	/* entity::controller::Delegate overrides                       */
	/* ************************************************************ */
	/* Global notifications */
	virtual void onTransportError(entity::controller::Interface const* const controller) noexcept override;
	/* Discovery Protocol (ADP) delegate */
	virtual void onEntityOnline(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept override;
	virtual void onEntityUpdate(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept override; // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	virtual void onEntityOffline(entity::controller::Interface const* const controller, UniqueIdentifier const entityID) noexcept override;
	/* Connection Management Protocol sniffed messages (ACMP) */
	virtual void onControllerConnectResponseSniffed(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onControllerDisconnectResponseSniffed(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onListenerConnectResponseSniffed(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onListenerDisconnectResponseSniffed(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onGetListenerStreamStateResponseSniffed(entity::controller::Interface const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
	virtual void onDeregisteredFromUnsolicitedNotifications(entity::controller::Interface const* const controller, UniqueIdentifier const entityID) noexcept override;
	virtual void onEntityAcquired(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onEntityReleased(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onEntityLocked(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, UniqueIdentifier const lockingEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onEntityUnlocked(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, UniqueIdentifier const lockingEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onConfigurationChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept override;
	virtual void onStreamInputFormatChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamOutputFormatChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamPortInputAudioMappingsChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamPortOutputAudioMappingsChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamInputInfoChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept override;
	virtual void onStreamOutputInfoChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept override;
	virtual void onEntityNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept override;
	virtual void onEntityGroupNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept override;
	virtual void onConfigurationNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept override;
	virtual void onAudioUnitNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) noexcept override;
	virtual void onStreamInputNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onStreamOutputNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onJackInputNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackName) noexcept override;
	virtual void onJackOutputNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackName) noexcept override;
	virtual void onAvbInterfaceNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) noexcept override;
	virtual void onClockSourceNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) noexcept override;
	virtual void onMemoryObjectNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) noexcept override;
	virtual void onAudioClusterNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) noexcept override;
	virtual void onControlNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName) noexcept override;
	virtual void onClockDomainNameChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) noexcept override;
	virtual void onAssociationIDChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, UniqueIdentifier const associationID) noexcept override;
	virtual void onAudioUnitSamplingRateChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept override;
	// onVideoClusterSamplingRateChanged
	// onSensorClusterSamplingRateChanged
	virtual void onClockSourceChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept override;
	virtual void onControlValuesChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues) noexcept override;
	virtual void onStreamInputStarted(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStarted(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamInputStopped(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStopped(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onAvbInfoChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept override;
	virtual void onAsPathChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath) noexcept override;
	virtual void onEntityCountersChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept override;
	virtual void onAvbInterfaceCountersChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept override;
	virtual void onClockDomainCountersChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept override;
	virtual void onStreamInputCountersChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept override;
	virtual void onStreamOutputCountersChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept override;
	virtual void onStreamPortInputAudioMappingsAdded(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamPortOutputAudioMappingsAdded(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamPortInputAudioMappingsRemoved(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamPortOutputAudioMappingsRemoved(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onMemoryObjectLengthChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept override;
	virtual void onOperationStatus(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete) noexcept override;
	/* Identification notifications */
	virtual void onEntityIdentifyNotification(entity::controller::Interface const* const controller, UniqueIdentifier const entityID) noexcept override;
	/* **** Statistics **** */
	virtual void onAecpRetry(entity::controller::Interface const* const controller, UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpTimeout(entity::controller::Interface const* const controller, UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpUnexpectedResponse(entity::controller::Interface const* const controller, UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpResponseTime(entity::controller::Interface const* const controller, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override;
	virtual void onAemAecpUnsolicitedReceived(entity::controller::Interface const* const controller, UniqueIdentifier const& entityID, la::avdecc::protocol::AecpSequenceID const sequenceID) noexcept override;

	/* ************************************************************ */
	/* Private methods used to update AEM and notify observers      */
	/* ************************************************************ */
	void updateEntity(ControlledEntityImpl& controlledEntity, entity::Entity const& entity) const noexcept;
	static void addCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag) noexcept;
	static void removeCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag) noexcept;
	void updateUnsolicitedNotificationsSubscription(ControlledEntityImpl& controlledEntity, bool const isSubscribed) const noexcept;
	void updateAcquiredState(ControlledEntityImpl& controlledEntity, model::AcquireState const acquireState, UniqueIdentifier const owningEntity) const noexcept;
	void updateLockedState(ControlledEntityImpl& controlledEntity, model::LockState const lockState, UniqueIdentifier const lockingEntity) const noexcept;
	void updateConfiguration(entity::controller::Interface const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const streamFormatRequired, bool const milanExtendedRequired, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const streamFormatRequired, bool const milanExtendedRequired, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAudioUnitName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateJackInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateJackOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAvbInterfaceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateClockSourceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateMemoryObjectName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAudioClusterName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateControlName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateClockDomainName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAssociationID(ControlledEntityImpl& controlledEntity, std::optional<UniqueIdentifier> const associationID, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	bool updateControlValues(ControlledEntityImpl& controlledEntity, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateGptpInformation(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, networkInterface::MacAddress const& macAddress, UniqueIdentifier const& gptpGrandmasterID, std::uint8_t const gptpDomainNumber, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAsPath(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	static void updateAvbInterfaceLinkStatus(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, ControlledEntity::InterfaceLinkStatus const linkStatus) noexcept;
	void updateEntityCounters(ControlledEntityImpl& controlledEntity, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateAvbInterfaceCounters(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateClockDomainCounters(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamInputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamOutputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateMemoryObjectLength(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	void updateOperationStatus(ControlledEntityImpl& controlledEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept;
	static void updateRedundancyWarning(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, bool const isWarning) noexcept;
	static void updateControlCurrentValueOutOfBounds(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ControlIndex const controlIndex, bool const isOutOfBounds) noexcept;
	void updateStreamInputLatency(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isOverLatency) const noexcept;

	/* ************************************************************ */
	/* Private classes                                              */
	/* ************************************************************ */
	/** A guard around a ControlledEntityImpl that guarantees it won't be destroyed while the Guard is alive. If 'locked' is passed during construction, it also guarantees it won't be modified by another thread while the guard is alive. */
	class ControlledEntityImplGuard final
	{
	public:
		/** Returns a ControlledEntity const* */
		ControlledEntityImpl const* get() const noexcept
		{
			if (_controlledEntity == nullptr)
				return nullptr;
			return _controlledEntity.get();
		}

		/** Returns a ControlledEntity* */
		ControlledEntityImpl* get() noexcept
		{
			if (_controlledEntity == nullptr)
				return nullptr;
			return _controlledEntity.get();
		}

		/** Operator to access ControlledEntity */
		ControlledEntityImpl const* operator->() const noexcept
		{
			if (_controlledEntity == nullptr)
				return nullptr;
			return _controlledEntity.operator->();
		}

		/** Operator to access ControlledEntity */
		ControlledEntityImpl* operator->() noexcept
		{
			if (_controlledEntity == nullptr)
				return nullptr;
			return _controlledEntity.operator->();
		}

		/** Operator to access ControlledEntity */
		ControlledEntityImpl const& operator*() const
		{
			if (_controlledEntity == nullptr)
				throw la::avdecc::Exception("ControlledEntityImpl is nullptr");
			return _controlledEntity.operator*();
		}

		/** Operator to access ControlledEntity */
		ControlledEntityImpl& operator*()
		{
			if (_controlledEntity == nullptr)
				throw la::avdecc::Exception("ControlledEntityImpl is nullptr");
			return _controlledEntity.operator*();
		}

		/** Returns true if the entity is online (meaning a valid ControlledEntity can be retrieved using an operator overload) */
		explicit operator bool() const noexcept
		{
			return _controlledEntity != nullptr;
		}

		/** Releases the Guarded SharedControlledEntityImpl, transfering ownership and locked state */
		SharedControlledEntityImpl release() noexcept
		{
			return std::move(_controlledEntity);
		}

		/** Releases the Guarded SharedControlledEntityImpl, unlocking it if it was */
		void reset() noexcept
		{
			unlock();
			_controlledEntity = nullptr;
		}

		// Default constructor to allow creation of an empty Guard
		ControlledEntityImplGuard() noexcept {}

		ControlledEntityImplGuard(SharedControlledEntityImpl&& entity, bool const locked)
			: _controlledEntity(std::move(entity))
			, _locked(locked)
		{
			if (_controlledEntity && _locked)
			{
				_controlledEntity->lock();
			}
		}

		// Destructor
		~ControlledEntityImplGuard()
		{
			unlock();
		}

		// Allow move semantics
		ControlledEntityImplGuard(ControlledEntityImplGuard&&) = default;
		ControlledEntityImplGuard& operator=(ControlledEntityImplGuard&&) = default;

		// Disallow copy
		ControlledEntityImplGuard(ControlledEntityImplGuard const&) = delete;
		ControlledEntityImplGuard& operator=(ControlledEntityImplGuard const&) = delete;

	private:
		void unlock() noexcept
		{
			if (_controlledEntity && _locked)
			{
				_controlledEntity->unlock();
				_locked = false;
			}
		}

		SharedControlledEntityImpl _controlledEntity{ nullptr };
		bool _locked{ false };
	};

	/** A guard around a ControlledEntityImpl that guarantees it won't be destroyed while the Guard is alive. All access to the underlying object is blocked. */
	class SharedControlledEntityImplHolder final
	{
	public:
		// Default constructor to allow creation of an empty Guard
		SharedControlledEntityImplHolder() noexcept {}

		SharedControlledEntityImplHolder(SharedControlledEntityImpl const& entity)
			: _controlledEntity(entity)
		{
		}

		/** Returns true if the entity is online */
		explicit operator bool() const noexcept
		{
			return _controlledEntity != nullptr;
		}

		/** Releases the Guarded SharedControlledEntityImpl, transfering ownership */
		SharedControlledEntityImpl release() noexcept
		{
			return std::move(_controlledEntity);
		}

		/** Releases the Guarded SharedControlledEntityImpl */
		void reset() noexcept
		{
			_controlledEntity = nullptr;
		}

		// Allow move semantics
		SharedControlledEntityImplHolder(SharedControlledEntityImplHolder&&) = default;
		SharedControlledEntityImplHolder& operator=(SharedControlledEntityImplHolder&&) = default;

		// Disallow copy
		SharedControlledEntityImplHolder(SharedControlledEntityImplHolder const&) = delete;
		SharedControlledEntityImplHolder& operator=(SharedControlledEntityImplHolder const&) = delete;

	private:
		SharedControlledEntityImpl _controlledEntity{ nullptr };
	};

	/** A guard around a ControllerImpl that guarantees the lock on all the ControlledEntities will be released during the lifetime of this object. When destroyed, all the locked count will be restored. */
	class ControlledEntityUnlockerGuard final
	{
	public:
		ControlledEntityUnlockerGuard(ControllerImpl const& controller) noexcept
			: _sharedLockInformation(controller._entitiesSharedLockInformation)
			, _wasLocked(_sharedLockInformation->isSelfLocked())
		{
			if (_wasLocked)
			{
				_lockedCount = _sharedLockInformation->unlockAll();
			}
		}

		~ControlledEntityUnlockerGuard() noexcept
		{
			if (_wasLocked)
			{
				_sharedLockInformation->lockAll(_lockedCount);
			}
		}

		// Allow move semantics
		ControlledEntityUnlockerGuard(ControlledEntityUnlockerGuard&&) = default;
		ControlledEntityUnlockerGuard& operator=(ControlledEntityUnlockerGuard&&) = default;

		// Disallow copy
		ControlledEntityUnlockerGuard(ControlledEntityUnlockerGuard const&) = delete;
		ControlledEntityUnlockerGuard& operator=(ControlledEntityUnlockerGuard const&) = delete;

	private:
		ControlledEntityImpl::LockInformation::SharedPointer _sharedLockInformation{ nullptr };
		bool _wasLocked{ false };
		std::uint32_t _lockedCount{ 0u };
	};

	/* ************************************************************ */
	/* Private enums                                                */
	/* ************************************************************ */
	enum class FailureAction
	{
		NotAuthenticated, /**< This query requires authentication. Caller should decide whether to retry with authentication or not. */
		TimedOut, /**< This query timed out, try it again. */
		Busy, /**< Device is busy, try it again. */
		NotSupported, /**< This query is not supported by the entity. Caller should decide whether to continue or not. */
		BadArguments, /**< This query is either not supported, or had an error. Called should decide whether to continue or not. */
		WarningContinue, /**< This query had a warning, but ignore and continue to next one. */
		ErrorContinue, /**< This query had an error, flag it, ignore and continue to next one. */
		MisbehaveContinue, /**< The entity misbehaved, flag it, ignore and continue to next one. */
		ErrorFatal, /**< This query returned a fatal error, enumeration should be stopped immediately. */
	};
	enum class DynamicControlValuesValidationResult
	{
		Valid,
		CurrentValueOutOfRange, /**< current value is out of min-max range - This is not considered an error for some ControlType */
		InvalidValues, /**< Either static or dynamic values are deemed invalid - This is a non-1722.1 compliance */
	};

	/* ************************************************************ */
	/* Private types                                                */
	/* ************************************************************ */
	using DelayedQueryHandler = std::function<void(entity::ControllerEntity*)>;
	struct DelayedQuery
	{
		std::chrono::time_point<std::chrono::system_clock> sendTime{};
		UniqueIdentifier entityID{ UniqueIdentifier::getUninitializedUniqueIdentifier() };
		DelayedQueryHandler queryHandler{};
	};
	using DelayedQueries = std::deque<DelayedQuery>;
	using StartOperationHandler = std::function<void(controller::ControlledEntity const* const entity, entity::ControllerEntity::AemCommandStatus const status, entity::model::OperationID const operationID, MemoryBuffer const& memoryBuffer)>;
	struct ControllerIdentificationState
	{
		std::chrono::time_point<std::chrono::system_clock> expireTime{};
		entity::model::ControlIndex controlIndex{};
	};

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	static entity::model::ControlValues makeIdentifyControlValues(bool const isEnabled) noexcept;
	static std::optional<bool> getIdentifyControlValue(entity::model::ControlValues const& values) noexcept;
	static void checkAvbInterfaceLinkStatus(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInterfaceCounters const& avbInterfaceCounters) noexcept;
	static void checkRedundancyWarningDiagnostics(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity) noexcept;
	void removeExclusiveAccessTokens(UniqueIdentifier const entityID, ExclusiveAccessToken::AccessType const type) const noexcept;
	bool areControlledEntitiesSelfLocked() const noexcept;
	std::tuple<model::AcquireState, UniqueIdentifier> getAcquiredInfoFromStatus(ControlledEntityImpl& entity, UniqueIdentifier const owningEntity, entity::ControllerEntity::AemCommandStatus const status, bool const releaseEntityResult) const noexcept;
	std::tuple<model::LockState, UniqueIdentifier> getLockedInfoFromStatus(ControlledEntityImpl& entity, UniqueIdentifier const lockingEntity, entity::ControllerEntity::AemCommandStatus const status, bool const unlockEntityResult) const noexcept;
	void addDelayedQuery(std::chrono::milliseconds const delay, UniqueIdentifier const entityID, DelayedQueryHandler&& queryHandler) noexcept;
	static void chooseLocale(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, std::string const& preferedLocale, std::function<void(entity::model::StringsIndex const stringsIndex)> const& missingStringsHandler) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, ControlledEntityImpl::MilanInfoType const milanInfoType, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex = std::uint16_t{ 0u }, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void getMilanInfo(ControlledEntityImpl* const entity) noexcept;
	void registerUnsol(ControlledEntityImpl* const entity) noexcept;
	void unregisterUnsol(ControlledEntityImpl* const entity) noexcept;
	void getStaticModel(ControlledEntityImpl* const entity) noexcept;
	void getDynamicInfo(ControlledEntityImpl* const entity) noexcept;
	void getDescriptorDynamicInfo(ControlledEntityImpl* const entity) noexcept;
	void checkEnumerationSteps(ControlledEntityImpl* const entity) noexcept;
	template<entity::model::DescriptorType StreamPortType>
	entity::model::AudioMappings validateMappings(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
	{
		static_assert(StreamPortType == entity::model::DescriptorType::StreamPortInput || StreamPortType == entity::model::DescriptorType::StreamPortOutput, "Can only validate StreamPortInput and StreamPortOutput");

		try
		{
			auto const& entity = controlledEntity.getEntity();
			if constexpr (StreamPortType == entity::model::DescriptorType::StreamPortInput)
			{
				auto const maxSinks = entity.getCommonInformation().listenerStreamSinks;
				auto const& streamPortNode = controlledEntity.getStreamPortInputNode(controlledEntity.getCurrentConfigurationIndex(), streamPortIndex);
				return validateMappings(controlledEntity, maxSinks, streamPortNode.staticModel.numberOfClusters, mappings);
			}
			else if constexpr (StreamPortType == entity::model::DescriptorType::StreamPortOutput)
			{
				auto const maxSources = entity.getCommonInformation().talkerStreamSources;
				auto const& streamPortNode = controlledEntity.getStreamPortOutputNode(controlledEntity.getCurrentConfigurationIndex(), streamPortIndex);
				return validateMappings(controlledEntity, maxSources, streamPortNode.staticModel.numberOfClusters, mappings);
			}
		}
		catch (...)
		{
			return {};
		}
	}
	entity::model::AudioMappings validateMappings(ControlledEntityImpl& controlledEntity, std::uint16_t const maxStreams, std::uint16_t const maxClusters, entity::model::AudioMappings const& mappings) const noexcept;
	static bool validateIdentifyControl(ControlledEntityImpl& controlledEntity, model::ControlNode const& identifyControlNode) noexcept;
	static DynamicControlValuesValidationResult validateControlValues(UniqueIdentifier const entityID, entity::model::ControlIndex const controlIndex, UniqueIdentifier const& controlType, entity::model::ControlValueType::Type const controlValueType, entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept;
	static void validateControlDescriptors(ControlledEntityImpl& controlledEntity) noexcept;
	static void validateRedundancy(ControlledEntityImpl& controlledEntity) noexcept;
	static void validateEntityModel(ControlledEntityImpl& controlledEntity) noexcept;
	static void validateEntity(ControlledEntityImpl& controlledEntity) noexcept;
	void computeAndUpdateMediaClockChain(ControlledEntityImpl& controlledEntity, model::ClockDomainNode& clockDomainNode, UniqueIdentifier const continueFromEntityID, entity::model::ClockDomainIndex const continueFromEntityDomainIndex, std::optional<entity::model::StreamIndex> const continueFromStreamOutputIndex, UniqueIdentifier const beingAdvertisedEntity) const noexcept;
	void onPreAdvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept;
	void onPostAdvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept;
	void onPreUnadvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept;
	FailureAction getFailureActionForMvuCommandStatus(entity::ControllerEntity::MvuCommandStatus const status) const noexcept;
	FailureAction getFailureActionForAemCommandStatus(entity::ControllerEntity::AemCommandStatus const status) const noexcept;
	FailureAction getFailureActionForControlStatus(entity::ControllerEntity::ControlStatus const status) const noexcept;
	bool processRegisterUnsolFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity) noexcept;
	bool processGetMilanModelFailureStatus(entity::ControllerEntity::MvuCommandStatus const status, ControlledEntityImpl* const entity, ControlledEntityImpl::MilanInfoType const milanInfoType, bool const optionalForMilan = false) noexcept;
	bool processGetStaticModelFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	bool processGetAecpDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex, bool const optionalForMilan) noexcept;
	bool processGetAcmpDynamicInfoFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, bool const optionalForMilan) noexcept;
	bool processGetAcmpDynamicInfoFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex, bool const optionalForMilan) noexcept;
	bool processGetDescriptorDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, bool const optionalForMilan) noexcept;
	bool fetchCorrespondingDescriptor(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	void handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept;
	void handleTalkerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept;
	void clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const;
	void addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const;
#ifdef ENABLE_AVDECC_FEATURE_JSON
	static SharedControlledEntityImpl loadControlledEntityFromJson(nlohmann::json const& object, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo);
	std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> registerVirtualControlledEntity(SharedControlledEntityImpl&& controlledEntity) noexcept;
	static SharedControlledEntityImpl createControlledEntityFromJson(nlohmann::json const& object, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo); // Throws DeserializationException
	static std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, std::vector<SharedControlledEntityImpl>> deserializeJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo) noexcept;
	static std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, SharedControlledEntityImpl> deserializeJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo) noexcept;
	static void setupDetachedVirtualControlledEntity(ControlledEntityImpl& entity) noexcept;
#endif // ENABLE_AVDECC_FEATURE_JSON
	entity::addressAccess::Tlv makeNextReadDeviceMemoryTlv(std::uint64_t const baseAddress, std::uint64_t const length, std::uint64_t const currentSize) const noexcept;
	entity::addressAccess::Tlv makeNextWriteDeviceMemoryTlv(std::uint64_t const baseAddress, DeviceMemoryBuffer const& memoryBuffer, std::uint64_t const currentSize) const noexcept;
	void onUserReadDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs, std::uint64_t const baseAddress, std::uint64_t const length, ReadDeviceMemoryProgressHandler const& progressHandler, ReadDeviceMemoryCompletionHandler const& completionHandler, DeviceMemoryBuffer&& memoryBuffer) const noexcept;
	void onUserWriteDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, std::uint64_t const baseAddress, std::uint64_t const sentSize, WriteDeviceMemoryProgressHandler const& progressHandler, WriteDeviceMemoryCompletionHandler const& completionHandler, DeviceMemoryBuffer&& memoryBuffer) const noexcept;
	void startOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept;
	void startMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartMemoryObjectOperationHandler const& handler) const noexcept;
	constexpr Controller& getSelf() const noexcept
	{
		return *const_cast<Controller*>(static_cast<Controller const*>(this));
	}

	/** Gets a scoped reference on a ControlledEntitiyImpl */
	inline SharedControlledEntityImplHolder getSharedControlledEntityImplHolder(UniqueIdentifier const entityID, bool const onlyIfAdvertised = false) const noexcept
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto const entityIt = _controlledEntities.find(entityID);
		if (entityIt != _controlledEntities.end())
		{
			if (!onlyIfAdvertised || entityIt->second->wasAdvertised())
			{
				// Return a reference on the entity while locked
				return SharedControlledEntityImplHolder{ entityIt->second };
			}
		}

		return {};
	}

	inline ControlledEntityImplGuard getControlledEntityImplGuard(UniqueIdentifier const entityID, bool const onlyIfAdvertised = false, bool const locked = true) const noexcept
	{
		auto entity = SharedControlledEntityImpl{};

		{
			// Lock to protect _controlledEntities
			std::lock_guard<decltype(_lock)> const lg(_lock);

			auto const entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				if (!onlyIfAdvertised || entityIt->second->wasAdvertised())
				{
					// Get a reference on the entity while locked
					entity = entityIt->second;
				}
			}
		}

		return ControlledEntityImplGuard{ std::move(entity), locked };
	}

	/* ************************************************************ */
	/* Private members                                              */
	/* ************************************************************ */
	mutable std::recursive_mutex _lock{}; // A mutex to protect all sensitive data members
	ControlledEntityImpl::LockInformation::SharedPointer _entitiesSharedLockInformation{ std::make_shared<ControlledEntityImpl::LockInformation>() }; // The SharedLockInformation to be used by all managed ControlledEntities
	std::unordered_map<UniqueIdentifier, SharedControlledEntityImpl, UniqueIdentifier::hash> _controlledEntities;
	EndStation::UniquePointer _endStation{ nullptr, nullptr };
	entity::ControllerEntity* _controller{ nullptr };
	std::unique_ptr<ControllerVirtualProxy> _controllerProxy{ nullptr };
	std::string _preferedLocale{ "en-US" };
	bool _fullStaticModelEnumeration{ false };
	bool _shouldTerminate{ false };
	DelayedQueries _delayedQueries{};
	std::unordered_map<UniqueIdentifier, std::chrono::time_point<std::chrono::system_clock>, UniqueIdentifier::hash> _entityIdentifications{}; // Holds Entity to Controller Identification Information
	mutable std::unordered_map<UniqueIdentifier, ControllerIdentificationState, UniqueIdentifier::hash> _controllerIdentifications{}; // Holds Controller to Entity Identification Information
	mutable std::unordered_map<UniqueIdentifier, std::set<ExclusiveAccessTokenImpl*>, UniqueIdentifier::hash> _exclusiveAccessTokens{};
	std::thread _stateMachinesThread{};
};

/* ************************************************************************** */
/* ExclusiveAccessTokenImpl class definition                                  */
/* ************************************************************************** */
class ExclusiveAccessTokenImpl final : public Controller::ExclusiveAccessToken
{
public:
	static UniquePointer create(ControllerImpl const* const controller, la::avdecc::UniqueIdentifier const entityID, AccessType const type)
	{
		auto deleter = [](ExclusiveAccessToken* self)
		{
			static_cast<ExclusiveAccessTokenImpl*>(self)->destroy();
		};
		return UniquePointer(new ExclusiveAccessTokenImpl(controller, entityID, type), deleter);
	}

	AccessType getAccessType() const noexcept
	{
		return _type;
	}

	void invalidateToken() noexcept
	{
		auto const lg = std::lock_guard{ _lock };

		_controller = nullptr;
	}

private:
	ExclusiveAccessTokenImpl(ControllerImpl const* const controller, la::avdecc::UniqueIdentifier const entityID, AccessType const type) noexcept
		: _controller(controller)
		, _entityID(entityID)
		, _type(type)
	{
	}
	virtual ~ExclusiveAccessTokenImpl() override
	{
		auto const lg = std::lock_guard{ _lock };

		if (_controller)
		{
			_controller->unregisterExclusiveAccessToken(_entityID, this);
		}
	}
	void destroy() noexcept
	{
		delete this;
	}

	// Private members
	std::mutex _lock{};
	ControllerImpl const* _controller{ nullptr };
	la::avdecc::UniqueIdentifier _entityID{ la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier() };
	AccessType _type{ AccessType::Acquire };
};

} // namespace controller
} // namespace avdecc
} // namespace la
