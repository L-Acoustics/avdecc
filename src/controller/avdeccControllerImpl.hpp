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
#include "la/avdecc/memoryBuffer.hpp"
#include "avdeccControlledEntityImpl.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <chrono>
#include <deque>

namespace la
{
namespace avdecc
{
namespace controller
{

class ControllerImpl final : public Controller, private entity::ControllerEntity::Delegate
{
public:
	ControllerImpl(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale);

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
	virtual void enableEntityModelCache() noexcept override;
	virtual void disableEntityModelCache() noexcept override;

	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& name, SetAudioUnitNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& name, SetAvbInterfaceNameHandler const& handler) const noexcept override;
	virtual void setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& name, SetClockSourceNameHandler const& handler) const noexcept override;
	virtual void setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& name, SetMemoryObjectNameHandler const& handler) const noexcept override;
	virtual void setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& name, SetAudioClusterNameHandler const& handler) const noexcept override;
	virtual void setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& name, SetClockDomainNameHandler const& handler) const noexcept override;
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
	virtual void startUploadOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint64_t const dataLength, StartOperationHandler const& handler) const override;
	virtual void setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept override;

	/* Enumeration and Control Protocol (AECP) AA */
	virtual void readDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, std::uint64_t const length, ReadDeviceMemoryHandler const& handler) const noexcept override;
	virtual void writeDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, DeviceMemoryBuffer memoryBuffer, WriteDeviceMemoryHandler const& handler) const noexcept override;

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
	void onStringsDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor) noexcept;
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
	void onConfigurationNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept;
	void onAudioUnitNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) noexcept;
	void onAudioUnitSamplingRateResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onInputStreamNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) noexcept;
	void onInputStreamFormatResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onOutputStreamNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) noexcept;
	void onOutputStreamFormatResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onAvbInterfaceNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) noexcept;
	void onClockSourceNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) noexcept;
	void onMemoryObjectNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) noexcept;
	void onMemoryObjectLengthResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept;
	void onAudioClusterNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) noexcept;
	void onClockDomainNameResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) noexcept;
	void onClockDomainSourceIndexResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::ConfigurationIndex const configurationIndex) noexcept;

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
	virtual void onTransportError(entity::ControllerEntity const* const controller) noexcept override;
	/* Discovery Protocol (ADP) delegate */
	virtual void onEntityOnline(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept override;
	virtual void onEntityUpdate(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept override; // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	virtual void onEntityOffline(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID) noexcept override;
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
	virtual void onAudioUnitNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) noexcept override;
	virtual void onStreamInputNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onStreamOutputNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onAvbInterfaceNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) noexcept override;
	virtual void onClockSourceNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) noexcept override;
	virtual void onMemoryObjectNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) noexcept override;
	virtual void onAudioClusterNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) noexcept override;
	virtual void onClockDomainNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) noexcept override;
	virtual void onAudioUnitSamplingRateChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept override;
	virtual void onClockSourceChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept override;
	virtual void onStreamInputStarted(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStarted(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamInputStopped(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStopped(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onAvbInfoChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept override;
	virtual void onMemoryObjectLengthChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept override;
	virtual void onOperationStatus(entity::ControllerEntity const* const controller, UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const operationID, std::uint16_t const percentComplete) noexcept override;

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
	void updateAudioUnitName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) const noexcept;
	void updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const noexcept;
	void updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const noexcept;
	void updateAvbInterfaceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) const noexcept;
	void updateClockSourceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) const noexcept;
	void updateMemoryObjectName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) const noexcept;
	void updateAudioClusterName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) const noexcept;
	void updateClockDomainName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) const noexcept;
	void updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const noexcept;
	void updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const noexcept;
	void updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept;
	void updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept;
	void updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, bool const alsoUpdateEntity = true) const noexcept;
	void updateMemoryObjectLength(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) const noexcept;
	void updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept;
	void updateOperationStatus(ControlledEntityImpl& controlledEntity, UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const operationID, std::uint16_t const percentComplete) const noexcept;

	/* ************************************************************ */
	/* Private enums                                                */
	/* ************************************************************ */
	enum class FailureAction
	{
		Ignore, /**< Ignore this query and continue to next one. */
		Retry, /**< Should retry this query. */
		NotSupported, /**< This query is not supported by the entity. Caller should decide whether to continue or not. */
		Fatal, /**< This query returned a fatal error, enumeration should be stopped immediately. */
	};

	/* ************************************************************ */
	/* Private types                                                */
	/* ************************************************************ */
	using DelayedQueryHandler = std::function<void(entity::ControllerEntity*)>;
	struct DelayedQuery
	{
		std::chrono::time_point<std::chrono::system_clock> sendTime{};
		la::avdecc::UniqueIdentifier entityID{ la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier() };
		DelayedQueryHandler queryHandler{};
	};
	using DelayedQueries = std::deque<DelayedQuery>;

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	void addDelayedQuery(std::chrono::milliseconds const delay, la::avdecc::UniqueIdentifier const entityID, DelayedQueryHandler&& queryHandler) noexcept;
	void chooseLocale(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex = std::uint16_t{ 0u }, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery = std::chrono::milliseconds{ 0 }) noexcept;
	void getMilanVersion(ControlledEntityImpl* const entity) noexcept;
	void registerUnsol(ControlledEntityImpl* const entity) noexcept;
	void getStaticModel(ControlledEntityImpl* const entity) noexcept;
	void getDynamicInfo(ControlledEntityImpl* const entity) noexcept;
	void getDescriptorDynamicInfo(ControlledEntityImpl* const entity) noexcept;
	void checkEnumerationSteps(ControlledEntityImpl* const entity) noexcept;
	FailureAction getFailureAction(entity::ControllerEntity::AemCommandStatus const status) const noexcept;
	FailureAction getFailureAction(entity::ControllerEntity::ControlStatus const status) const noexcept;
	bool processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	bool processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex = std::uint16_t{ 0u }) noexcept;
	bool processFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex = std::uint16_t{ 0u }) noexcept;
	bool processFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex) noexcept;
	bool processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	bool fetchCorrespondingDescriptor(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept;
	void handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept;
	void handleTalkerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept;
	void clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex) const noexcept;
	void addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept;
	void delTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept;
	entity::addressAccess::Tlv makeNextReadDeviceMemoryTlv(std::uint64_t const baseAddress, std::uint64_t const length, std::uint64_t const currentSize) const noexcept;
	entity::addressAccess::Tlv makeNextWriteDeviceMemoryTlv(std::uint64_t const baseAddress, DeviceMemoryBuffer const& memoryBuffer, std::uint64_t const currentSize) const noexcept;
	void onUserReadDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs, std::uint64_t const baseAddress, std::uint64_t const length, ReadDeviceMemoryHandler&& handler, DeviceMemoryBuffer&& memoryBuffer) const noexcept;
	void onUserWriteDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, std::uint64_t const baseAddress, std::uint64_t const sentSize, WriteDeviceMemoryHandler&& handler, DeviceMemoryBuffer&& memoryBuffer) const noexcept;
	void startOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept;
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
	mutable std::mutex _lock{}; // A mutex to protect _controlledEntities and _delayedQueries
	std::unordered_map<UniqueIdentifier, OnlineControlledEntity, UniqueIdentifier::hash> _controlledEntities;
	EndStation::UniquePointer _endStation{ nullptr, nullptr };
	entity::ControllerEntity* _controller{ nullptr };
	std::string _preferedLocale{ "en-US" };
	// Delayed queries variables
	bool _shouldTerminate{ false };
	DelayedQueries _delayedQueries{};
	std::thread _delayedQueryThread{};
};

} // namespace controller
} // namespace avdecc
} // namespace la
