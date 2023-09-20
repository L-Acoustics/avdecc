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
* @file controllerCapabilityDelegate.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/controllerEntity.hpp"

#include "entityImpl.hpp"
#include "aemHandler.hpp"

#include <chrono>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace controller
{
class CapabilityDelegate final : public entity::CapabilityDelegate
{
public:
	/* ************************************************************************** */
	/* CapabilityDelegate life cycle                                              */
	/* ************************************************************************** */
	CapabilityDelegate(protocol::ProtocolInterface* const protocolInterface, controller::Delegate* controllerDelegate, Interface& controllerInterface, Entity const& entity, model::EntityTree const* const entityModelTree);
	virtual ~CapabilityDelegate() noexcept;

	/* ************************************************************************** */
	/* Controller methods                                                         */
	/* ************************************************************************** */
	void setControllerDelegate(controller::Delegate* const delegate) noexcept;
	/* Discovery Protocol (ADP) */
	/* Enumeration and Control Protocol (AECP) AEM */
	void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::AcquireEntityHandler const& handler) const noexcept;
	void releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::ReleaseEntityHandler const& handler) const noexcept;
	void lockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::LockEntityHandler const& handler) const noexcept;
	void unlockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::UnlockEntityHandler const& handler) const noexcept;
	void queryEntityAvailable(UniqueIdentifier const targetEntityID, Interface::QueryEntityAvailableHandler const& handler) const noexcept;
	void queryControllerAvailable(UniqueIdentifier const targetEntityID, Interface::QueryControllerAvailableHandler const& handler) const noexcept;
	void registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, Interface::RegisterUnsolicitedNotificationsHandler const& handler) const noexcept;
	void unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, Interface::UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept;
	void readEntityDescriptor(UniqueIdentifier const targetEntityID, Interface::EntityDescriptorHandler const& handler) const noexcept;
	void readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, Interface::ConfigurationDescriptorHandler const& handler) const noexcept;
	void readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, Interface::AudioUnitDescriptorHandler const& handler) const noexcept;
	void readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::StreamInputDescriptorHandler const& handler) const noexcept;
	void readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::StreamOutputDescriptorHandler const& handler) const noexcept;
	void readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, Interface::JackInputDescriptorHandler const& handler) const noexcept;
	void readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, Interface::JackOutputDescriptorHandler const& handler) const noexcept;
	void readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::AvbInterfaceDescriptorHandler const& handler) const noexcept;
	void readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, Interface::ClockSourceDescriptorHandler const& handler) const noexcept;
	void readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, Interface::MemoryObjectDescriptorHandler const& handler) const noexcept;
	void readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, Interface::LocaleDescriptorHandler const& handler) const noexcept;
	void readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, Interface::StringsDescriptorHandler const& handler) const noexcept;
	void readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, Interface::StreamPortInputDescriptorHandler const& handler) const noexcept;
	void readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, Interface::StreamPortOutputDescriptorHandler const& handler) const noexcept;
	void readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, Interface::ExternalPortInputDescriptorHandler const& handler) const noexcept;
	void readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, Interface::ExternalPortOutputDescriptorHandler const& handler) const noexcept;
	void readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, Interface::InternalPortInputDescriptorHandler const& handler) const noexcept;
	void readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, Interface::InternalPortOutputDescriptorHandler const& handler) const noexcept;
	void readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, Interface::AudioClusterDescriptorHandler const& handler) const noexcept;
	void readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, Interface::AudioMapDescriptorHandler const& handler) const noexcept;
	void readControlDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, Interface::ControlDescriptorHandler const& handler) const noexcept;
	void readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, Interface::ClockDomainDescriptorHandler const& handler) const noexcept;
	void readTimingDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, Interface::TimingDescriptorHandler const& handler) const noexcept;
	void readPtpInstanceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, Interface::PtpInstanceDescriptorHandler const& handler) const noexcept;
	void readPtpPortDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, Interface::PtpPortDescriptorHandler const& handler) const noexcept;
	void setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, Interface::SetConfigurationHandler const& handler) const noexcept;
	void getConfiguration(UniqueIdentifier const targetEntityID, Interface::GetConfigurationHandler const& handler) const noexcept;
	void setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, Interface::SetStreamInputFormatHandler const& handler) const noexcept;
	void getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamInputFormatHandler const& handler) const noexcept;
	void setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, Interface::SetStreamOutputFormatHandler const& handler) const noexcept;
	void getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamOutputFormatHandler const& handler) const noexcept;
	void getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, Interface::GetStreamPortInputAudioMapHandler const& handler) const noexcept;
	void getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, Interface::GetStreamPortOutputAudioMapHandler const& handler) const noexcept;
	void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::AddStreamPortInputAudioMappingsHandler const& handler) const noexcept;
	void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept;
	void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept;
	void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept;
	void setStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, Interface::SetStreamInputInfoHandler const& handler) const noexcept;
	void setStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, Interface::SetStreamOutputInfoHandler const& handler) const noexcept;
	void getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamInputInfoHandler const& handler) const noexcept;
	void getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamOutputInfoHandler const& handler) const noexcept;
	void setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, Interface::SetEntityNameHandler const& handler) const noexcept;
	void getEntityName(UniqueIdentifier const targetEntityID, Interface::GetEntityNameHandler const& handler) const noexcept;
	void setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, Interface::SetEntityGroupNameHandler const& handler) const noexcept;
	void getEntityGroupName(UniqueIdentifier const targetEntityID, Interface::GetEntityGroupNameHandler const& handler) const noexcept;
	void setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& configurationName, Interface::SetConfigurationNameHandler const& handler) const noexcept;
	void getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, Interface::GetConfigurationNameHandler const& handler) const noexcept;
	void setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, Interface::SetAudioUnitNameHandler const& handler) const noexcept;
	void getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, Interface::GetAudioUnitNameHandler const& handler) const noexcept;
	void setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, Interface::SetStreamInputNameHandler const& handler) const noexcept;
	void getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::GetStreamInputNameHandler const& handler) const noexcept;
	void setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, Interface::SetStreamOutputNameHandler const& handler) const noexcept;
	void getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::GetStreamOutputNameHandler const& handler) const noexcept;
	void setJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackInputName, Interface::SetJackInputNameHandler const& handler) const noexcept;
	void getJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, Interface::GetJackInputNameHandler const& handler) const noexcept;
	void setJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackOutputName, Interface::SetJackOutputNameHandler const& handler) const noexcept;
	void getJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, Interface::GetJackOutputNameHandler const& handler) const noexcept;
	void setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, Interface::SetAvbInterfaceNameHandler const& handler) const noexcept;
	void getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAvbInterfaceNameHandler const& handler) const noexcept;
	void setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, Interface::SetClockSourceNameHandler const& handler) const noexcept;
	void getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, Interface::GetClockSourceNameHandler const& handler) const noexcept;
	void setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, Interface::SetMemoryObjectNameHandler const& handler) const noexcept;
	void getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, Interface::GetMemoryObjectNameHandler const& handler) const noexcept;
	void setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, Interface::SetAudioClusterNameHandler const& handler) const noexcept;
	void getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, Interface::GetAudioClusterNameHandler const& handler) const noexcept;
	void setControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, model::AvdeccFixedString const& controlName, Interface::SetControlNameHandler const& handler) const noexcept;
	void getControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, Interface::GetControlNameHandler const& handler) const noexcept;
	void setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, Interface::SetClockDomainNameHandler const& handler) const noexcept;
	void getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, Interface::GetClockDomainNameHandler const& handler) const noexcept;
	void setTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, model::AvdeccFixedString const& timingName, Interface::SetTimingNameHandler const& handler) const noexcept;
	void getTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, Interface::GetTimingNameHandler const& handler) const noexcept;
	void setPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, model::AvdeccFixedString const& ptpInstanceName, Interface::SetPtpInstanceNameHandler const& handler) const noexcept;
	void getPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, Interface::GetPtpInstanceNameHandler const& handler) const noexcept;
	void setPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, model::AvdeccFixedString const& ptpPortName, Interface::SetPtpPortNameHandler const& handler) const noexcept;
	void getPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, Interface::GetPtpPortNameHandler const& handler) const noexcept;
	void setAssociationID(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, Interface::SetAssociationHandler const& handler) const noexcept;
	void getAssociationID(UniqueIdentifier const targetEntityID, Interface::GetAssociationHandler const& handler) const noexcept;
	void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, Interface::SetAudioUnitSamplingRateHandler const& handler) const noexcept;
	void getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, Interface::GetAudioUnitSamplingRateHandler const& handler) const noexcept;
	void setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, Interface::SetVideoClusterSamplingRateHandler const& handler) const noexcept;
	void getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, Interface::GetVideoClusterSamplingRateHandler const& handler) const noexcept;
	void setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, Interface::SetSensorClusterSamplingRateHandler const& handler) const noexcept;
	void getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, Interface::GetSensorClusterSamplingRateHandler const& handler) const noexcept;
	void setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, Interface::SetClockSourceHandler const& handler) const noexcept;
	void getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, Interface::GetClockSourceHandler const& handler) const noexcept;
	void setControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, model::ControlValues const& controlValues, Interface::SetControlValuesHandler const& handler) const noexcept;
	void getControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, Interface::GetControlValuesHandler const& handler) const noexcept;
	void startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StartStreamInputHandler const& handler) const noexcept;
	void startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StartStreamOutputHandler const& handler) const noexcept;
	void stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StopStreamInputHandler const& handler) const noexcept;
	void stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StopStreamOutputHandler const& handler) const noexcept;
	void getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAvbInfoHandler const& handler) const noexcept;
	void getAsPath(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAsPathHandler const& handler) const noexcept;
	void getEntityCounters(UniqueIdentifier const targetEntityID, Interface::GetEntityCountersHandler const& handler) const noexcept;
	void getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAvbInterfaceCountersHandler const& handler) const noexcept;
	void getClockDomainCounters(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, Interface::GetClockDomainCountersHandler const& handler) const noexcept;
	void getStreamInputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamInputCountersHandler const& handler) const noexcept;
	void getStreamOutputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamOutputCountersHandler const& handler) const noexcept;
	void reboot(UniqueIdentifier const targetEntityID, Interface::RebootHandler const& handler) const noexcept;
	void rebootToFirmware(UniqueIdentifier const targetEntityID, model::MemoryObjectIndex const memoryObjectIndex, Interface::RebootToFirmwareHandler const& handler) const noexcept;
	void startOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, Interface::StartOperationHandler const& handler) const noexcept;
	void abortOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, Interface::AbortOperationHandler const& handler) const noexcept;
	void setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, Interface::SetMemoryObjectLengthHandler const& handler) const noexcept;
	void getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, Interface::GetMemoryObjectLengthHandler const& handler) const noexcept;
	/* Enumeration and Control Protocol (AECP) AA */
	void addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, Interface::AddressAccessHandler const& handler) const noexcept;
	/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
	void getMilanInfo(UniqueIdentifier const targetEntityID, Interface::GetMilanInfoHandler const& handler) const noexcept;
	/* Connection Management Protocol (ACMP) */
	void connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::ConnectStreamHandler const& handler) const noexcept;
	void disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectStreamHandler const& handler) const noexcept;
	void disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectTalkerStreamHandler const& handler) const noexcept;
	void getTalkerStreamState(model::StreamIdentification const& talkerStream, Interface::GetTalkerStreamStateHandler const& handler) const noexcept;
	void getListenerStreamState(model::StreamIdentification const& listenerStream, Interface::GetListenerStreamStateHandler const& handler) const noexcept;
	void getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, Interface::GetTalkerStreamConnectionHandler const& handler) const noexcept;

	/* ************************************************************************** */
	/* Controller notifications                                                   */
	/* ************************************************************************** */
	/* **** Statistics **** */
	void onAecpRetry(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept;
	void onAecpTimeout(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept;
	void onAecpUnexpectedResponse(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept;
	void onAecpResponseTime(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept;

	// Deleted compiler auto-generated methods
	CapabilityDelegate(CapabilityDelegate&&) = delete;
	CapabilityDelegate(CapabilityDelegate const&) = delete;
	CapabilityDelegate& operator=(CapabilityDelegate const&) = delete;
	CapabilityDelegate& operator=(CapabilityDelegate&&) = delete;

private:
	struct DiscoveredEntity
	{
		Entity entity;
		model::AvbInterfaceIndex mainInterfaceIndex{};

		DiscoveredEntity(Entity const& entity, model::AvbInterfaceIndex const mainInterfaceIndex)
			: entity{ entity }
			, mainInterfaceIndex{ mainInterfaceIndex }
		{
		}
	};
	using DiscoveredEntities = std::unordered_map<UniqueIdentifier, DiscoveredEntity, UniqueIdentifier::hash>;

	/* ************************************************************************** */
	/* CapabilityDelegate overrides                                               */
	/* ************************************************************************** */
	virtual void onTransportError(protocol::ProtocolInterface* const pi) noexcept override;
	/* **** Discovery notifications **** */
	virtual void onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	/* **** AECP notifications **** */
	virtual bool onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept override;
	virtual void onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept override;
	virtual void onAecpAemIdentifyNotification(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept override;
	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;

	/* ************************************************************************** */
	/* Internal methods                                                           */
	/* ************************************************************************** */
	model::AvbInterfaceIndex getMainInterfaceIndex(Entity const& entity) const noexcept;
	bool isResponseForController(protocol::AcmpMessageType const messageType) const noexcept;
	void sendAemAecpCommand(UniqueIdentifier const targetEntityID, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, LocalEntityImpl<>::OnAemAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void sendAaAecpCommand(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, LocalEntityImpl<>::OnAaAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void sendMvuAecpCommand(UniqueIdentifier const targetEntityID, protocol::MvuCommandType const commandType, void const* const payload, size_t const payloadLength, LocalEntityImpl<>::OnMvuAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionIndex, LocalEntityImpl<>::OnACMPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void processAemAecpResponse(protocol::AemCommandType const commandType, protocol::Aecpdu const* const response, LocalEntityImpl<>::OnAemAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void processAaAecpResponse(protocol::Aecpdu const* const response, LocalEntityImpl<>::OnAaAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void processMvuAecpResponse(protocol::MvuCommandType const commandType, protocol::Aecpdu const* const response, LocalEntityImpl<>::OnMvuAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void processAcmpResponse(protocol::Acmpdu const* const response, LocalEntityImpl<>::OnACMPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed) const noexcept;

	/* ************************************************************************** */
	/* Internal variables                                                         */
	/* ************************************************************************** */
	protocol::ProtocolInterface* const _protocolInterface{ nullptr };
	controller::Delegate* _controllerDelegate{ nullptr };
	Interface& _controllerInterface;
	UniqueIdentifier const _controllerID{ UniqueIdentifier::getNullUniqueIdentifier() };
	model::AemHandler const _aemHandler;
	DiscoveredEntities _discoveredEntities{};
};

} // namespace controller
} // namespace entity
} // namespace avdecc
} // namespace la
