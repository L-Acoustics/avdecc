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
* @file controllerEntityImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/controllerEntity.hpp"
#include "la/avdecc/internals/protocolInterface.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/internals/protocolMvuAecpdu.hpp"

#include "entityImpl.hpp"

#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace entity
{
/* ************************************************************************** */
/* ControllerEntityImpl                                                       */
/* ************************************************************************** */
class ControllerEntityImpl : public LocalEntityImpl<ControllerEntity>
{
private:
	friend class LocalEntityGuard<ControllerEntityImpl>; // The only way to construct ControllerEntityImpl, through LocalEntityGuard

	/* ************************************************************************** */
	/* ControllerEntityImpl life cycle                                            */
	/* ************************************************************************** */
	ControllerEntityImpl(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const controllerDelegate);
	virtual ~ControllerEntityImpl() noexcept;
	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override;

private:
	/* ************************************************************************** */
	/* controller::Interface overrides                                            */
	/* ************************************************************************** */
	/* Discovery Protocol (ADP) */
	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void lockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, LockEntityHandler const& handler) const noexcept override;
	virtual void unlockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, UnlockEntityHandler const& handler) const noexcept override;
	virtual void queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept override;
	virtual void queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept override;
	virtual void registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept override;
	virtual void unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept override;
	virtual void readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept override;
	virtual void readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept override;
	virtual void readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept override;
	virtual void readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept override;
	virtual void readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept override;
	virtual void readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept override;
	virtual void readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept override;
	virtual void readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept override;
	virtual void readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept override;
	virtual void readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept override;
	virtual void readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept override;
	virtual void readControlDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, ControlDescriptorHandler const& handler) const noexcept override;
	virtual void readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept override;
	virtual void readTimingDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, TimingDescriptorHandler const& handler) const noexcept override;
	virtual void readPtpInstanceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, PtpInstanceDescriptorHandler const& handler) const noexcept override;
	virtual void readPtpPortDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, PtpPortDescriptorHandler const& handler) const noexcept override;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept override;
	virtual void getConfiguration(UniqueIdentifier const targetEntityID, GetConfigurationHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept override;
	virtual void getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept override;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void setStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept override;
	virtual void setStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept override;
	virtual void getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept override;
	virtual void getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept override;
	virtual void getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& configurationName, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept override;
	virtual void getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void setJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackInputName, SetJackInputNameHandler const& handler) const noexcept override;
	virtual void getJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, GetJackInputNameHandler const& handler) const noexcept override;
	virtual void setJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackOutputName, SetJackOutputNameHandler const& handler) const noexcept override;
	virtual void getJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, GetJackOutputNameHandler const& handler) const noexcept override;
	virtual void setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept override;
	virtual void getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept override;
	virtual void setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept override;
	virtual void getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept override;
	virtual void setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept override;
	virtual void getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept override;
	virtual void setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept override;
	virtual void getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept override;
	virtual void setControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, model::AvdeccFixedString const& controlName, SetControlNameHandler const& handler) const noexcept override;
	virtual void getControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, GetControlNameHandler const& handler) const noexcept override;
	virtual void setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept override;
	virtual void getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept override;
	virtual void setTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, model::AvdeccFixedString const& timingName, SetTimingNameHandler const& handler) const noexcept override;
	virtual void getTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, GetTimingNameHandler const& handler) const noexcept override;
	virtual void setPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, model::AvdeccFixedString const& ptpInstanceName, SetPtpInstanceNameHandler const& handler) const noexcept override;
	virtual void getPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, GetPtpInstanceNameHandler const& handler) const noexcept override;
	virtual void setPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, model::AvdeccFixedString const& ptpPortName, SetPtpPortNameHandler const& handler) const noexcept override;
	virtual void getPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, GetPtpPortNameHandler const& handler) const noexcept override;
	virtual void setAssociation(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationHandler const& handler) const noexcept override;
	virtual void getAssociation(UniqueIdentifier const targetEntityID, GetAssociationHandler const& handler) const noexcept override;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept override;
	virtual void getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept override;
	virtual void setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept override;
	virtual void getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept override;
	virtual void setControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept override;
	virtual void getControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, GetControlValuesHandler const& handler) const noexcept override;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept override;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept override;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept override;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept override;
	virtual void getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept override;
	virtual void getAsPath(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAsPathHandler const& handler) const noexcept override;
	virtual void getEntityCounters(UniqueIdentifier const targetEntityID, GetEntityCountersHandler const& handler) const noexcept override;
	virtual void getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept override;
	virtual void getClockDomainCounters(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept override;
	virtual void getStreamInputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept override;
	virtual void getStreamOutputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputCountersHandler const& handler) const noexcept override;
	virtual void reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept override;
	virtual void rebootToFirmware(UniqueIdentifier const targetEntityID, model::MemoryObjectIndex const memoryObjectIndex, RebootToFirmwareHandler const& handler) const noexcept override;
	virtual void startOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept override;
	virtual void abortOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept override;
	virtual void setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept override;
	virtual void getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept override;
	/* Enumeration and Control Protocol (AECP) AA */
	virtual void addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept override;
	/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
	virtual void getMilanInfo(UniqueIdentifier const targetEntityID, GetMilanInfoHandler const& handler) const noexcept override;
	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept override;
	virtual void getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept override;
	virtual void getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept override;
	/* Other methods */
	virtual void setControllerDelegate(controller::Delegate* const delegate) noexcept override;
	controller::Delegate* getControllerDelegate() const noexcept;

	/* ************************************************************************** */
	/* protocol::ProtocolInterface::Observer overrides                            */
	/* ************************************************************************** */
	/* **** Global notifications **** */
	virtual void onTransportError(protocol::ProtocolInterface* const pi) noexcept override;
	/* **** Discovery notifications **** */
	virtual void onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	/* **** AECP notifications **** */
	// virtual void onAecpCommand(protocol::ProtocolInterface* const pi, LocalEntity const& entity, protocol::Aecpdu const& aecpdu) noexcept override; Already defined in base class LocalEntityImpl, dispatching through onUnhandledAecpCommand
	virtual void onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept override;
	virtual void onAecpAemIdentifyNotification(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept override;
	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;

	/* **** Statistics **** */
	virtual void onAecpRetry(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpTimeout(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpUnexpectedResponse(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpResponseTime(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override;

	/* ************************************************************************** */
	/* LocalEntityImpl overrides                                                  */
	/* ************************************************************************** */
	virtual bool onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept override;
	virtual bool onUnhandledAecpVuCommand(protocol::ProtocolInterface* const pi, protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, protocol::Aecpdu const& aecpdu) noexcept override;

	/* ************************************************************************** */
	/* Internal variables                                                         */
	/* ************************************************************************** */
	CapabilityDelegate::UniquePointer _controllerCapabilityDelegate{ nullptr };
};

} // namespace entity
} // namespace avdecc
} // namespace la
