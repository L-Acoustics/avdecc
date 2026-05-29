/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file avdeccControllerProxy.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/controller/avdeccController.hpp"

#include <la/avdecc/internals/controllerEntity.hpp>

#include <mutex>
#include <set>
#include <unordered_map>

namespace la
{
namespace avdecc
{
namespace controller
{
/** Proxy class to route all controller::Interface calls between the virtual and the real controller::Interface depending on the virtual entity state.
 *  In dual-interface (redundancy) mode, this proxy also routes between two real interfaces (Primary and Secondary) based on per-entity reachability.
 */
class ControllerVirtualProxy : public entity::controller::Interface
{
public:
	/** Per-entity reachability state across the (optional) two real interfaces. */
	struct InterfaceReachability
	{
		bool onPrimary{ false }; /**< True if the entity is currently reachable on the Primary interface. */
		bool onSecondary{ false }; /**< True if the entity is currently reachable on the Secondary interface. */
	};

	/** State of the unsolicited notification registration for a single (entity, PI) pair. */
	enum class UnsolState : std::uint8_t
	{
		NotRegistered, /**< No registration is currently in flight nor completed. */
		Pending, /**< A REGISTER_UNSOLICITED_NOTIFICATION command has been sent and is awaiting its response. */
		Registered, /**< The talker has confirmed the unsolicited subscription on this PI. */
	};

	/** Constructor (single-interface mode). */
	ControllerVirtualProxy(protocol::ProtocolInterface const* const protocolInterface, entity::controller::Interface const* const realInterface, entity::controller::Interface const* const virtualInterface) noexcept;

	/** Constructor (dual-interface mode for redundancy). The secondaryRealInterface may be nullptr to fall back to single-interface mode. */
	ControllerVirtualProxy(protocol::ProtocolInterface const* const protocolInterface, entity::controller::Interface const* const primaryRealInterface, entity::controller::Interface const* const secondaryRealInterface, entity::controller::Interface const* const virtualInterface) noexcept;

	/** Destructor */
	virtual ~ControllerVirtualProxy() noexcept;

	/** Sets the specified UniqueIdentifier as a virtual entity */
	void setVirtualEntity(UniqueIdentifier const& virtualEntity) noexcept;

	/** Clears the specified UniqueIdentifier as a virtual entity */
	void clearVirtualEntity(UniqueIdentifier const& virtualEntity) noexcept;

	/** Marks the specified entity as reachable (or not) on the given interface. Called by ControllerImpl from ADP callbacks.
	 * @details In dual-PI mode, also resets the per-PI unsolicited-notification state to NotRegistered when transitioning to unreachable, so the registration is re-issued the next time the PI comes back.
	 * @return True if this call caused a `false→true` transition on the given interface (the caller may use this to lazily re-register unsolicited notifications on the PI that just came back online).
	 */
	bool setEntityReachable(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType, bool const reachable) noexcept;

	/** Clears all reachability state for the specified entity (called when the entity goes fully offline). */
	void clearEntityReachability(UniqueIdentifier const& entityID) noexcept;

	/** Marks all entities as unreachable on the specified interface (called when a transport error is reported on that interface). Returns true if the other interface is still up, false if both PIs are now down (or single-PI mode). */
	bool markInterfaceDown(Controller::InterfaceType const interfaceType) noexcept;

	/** Returns the current reachability for the specified entity (both PIs false if entity is unknown). */
	InterfaceReachability getEntityReachability(UniqueIdentifier const& entityID) const noexcept;

	/** Records the set of AvbInterfaceIndex values last observed via the specified PI's ADP for the given entity. */
	void setEntityInterfaceIndices(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType, std::set<entity::model::AvbInterfaceIndex> indices) noexcept;

	/** Returns the set of AvbInterfaceIndex values last observed via the specified PI's ADP for the given entity. */
	std::set<entity::model::AvbInterfaceIndex> getEntityInterfaceIndices(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType) const noexcept;

	/** Returns true if the controller is operating in dual-interface mode. */
	bool isDualInterface() const noexcept;

	/** Returns the secondary real interface (or nullptr if single-interface mode). */
	entity::controller::Interface const* getSecondaryRealInterface() const noexcept;

	/** Returns the "other" real interface (the one that was not chosen), or nullptr in single-interface mode. */
	entity::controller::Interface const* otherRealInterface(entity::controller::Interface const* const chosenInterface) const noexcept;

	/** Returns the "other" real interface only if the target entity is currently reachable on it (i.e. it makes sense to retry there).
	 * Returns nullptr in single-interface mode or when the other PI does not have reachability for @a entityID. This is used by the retry layer
	 * to avoid converting a legitimate failure on one PI (e.g. TimedOut, BadArguments) into a misleading UnknownEntity reported by the other PI.
	 */
	entity::controller::Interface const* otherReachableInterface(UniqueIdentifier const& entityID, entity::controller::Interface const* const chosenInterface) const noexcept;

	/** Atomically transitions the unsol state for (@a entityID, @a interfaceType) from NotRegistered to Pending and returns true if the caller "owns" the transition (i.e. should send the register command now). Returns false otherwise (already Pending or Registered, or in single-PI mode and @a interfaceType is Secondary). */
	bool tryClaimUnsolPending(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType) noexcept;

	/** Sets the unsol state for the (@a entityID, @a interfaceType) pair. Typically called from the per-PI register result handler. */
	void setUnsolState(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType, UnsolState const state) noexcept;

	/** Returns the current unsol state for the (@a entityID, @a interfaceType) pair. */
	UnsolState getUnsolState(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType) const noexcept;

	/** Sends a REGISTER_UNSOLICITED_NOTIFICATION command directly on the specified real PI, bypassing the proxy's pickRealInterface and the dual-PI retry layer.
	 * @details This is used to register the talker's unsolicited subscription on each reachable PI independently, so that loss of one PI does not silently drop unsolicited notifications.
	 * @note In single-PI mode, the command is always sent on the primary interface regardless of @a interfaceType.
	 */
	void registerUnsolicitedNotificationsOnInterface(UniqueIdentifier const targetEntityID, Controller::InterfaceType const interfaceType, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept;

	/** Returns true if the AEM command status indicates the command should be retried on the fallback interface in dual-interface mode. */
	static bool shouldRetry(entity::ControllerEntity::AemCommandStatus const status) noexcept;
	/** Returns true if the AA command status indicates the command should be retried on the fallback interface in dual-interface mode. */
	static bool shouldRetry(entity::ControllerEntity::AaCommandStatus const status) noexcept;
	/** Returns true if the MVU command status indicates the command should be retried on the fallback interface in dual-interface mode. */
	static bool shouldRetry(entity::ControllerEntity::MvuCommandStatus const status) noexcept;
	/** Returns true if the ACMP control status indicates the command should be retried on the fallback interface in dual-interface mode. */
	static bool shouldRetry(entity::ControllerEntity::ControlStatus const status) noexcept;

	// entity::controller::Interface overrides
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void lockEntity(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, LockEntityHandler const& handler) const noexcept override;
	virtual void unlockEntity(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, UnlockEntityHandler const& handler) const noexcept override;
	virtual void queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept override;
	virtual void queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept override;
	virtual void registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept override;
	virtual void unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept override;
	virtual void readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept override;
	virtual void readConfigurationDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept override;
	virtual void readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readJackInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept override;
	virtual void readJackOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept override;
	virtual void readClockSourceDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept override;
	virtual void readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept override;
	virtual void readLocaleDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept override;
	virtual void readStringsDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept override;
	virtual void readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept override;
	virtual void readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept override;
	virtual void readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept override;
	virtual void readAudioMapDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept override;
	virtual void readControlDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, ControlDescriptorHandler const& handler) const noexcept override;
	virtual void readClockDomainDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept override;
	virtual void readTimingDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, TimingDescriptorHandler const& handler) const noexcept override;
	virtual void readPtpInstanceDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, PtpInstanceDescriptorHandler const& handler) const noexcept override;
	virtual void readPtpPortDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, PtpPortDescriptorHandler const& handler) const noexcept override;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept override;
	virtual void getConfiguration(UniqueIdentifier const targetEntityID, GetConfigurationHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void getStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void getStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept override;
	virtual void getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept override;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void setStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept override;
	virtual void setStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept override;
	virtual void getStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept override;
	virtual void getStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept override;
	virtual void getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void getConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept override;
	virtual void getAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void getStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void getStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void setJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName, SetJackInputNameHandler const& handler) const noexcept override;
	virtual void getJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, GetJackInputNameHandler const& handler) const noexcept override;
	virtual void setJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName, SetJackOutputNameHandler const& handler) const noexcept override;
	virtual void getJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, GetJackOutputNameHandler const& handler) const noexcept override;
	virtual void setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept override;
	virtual void getAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept override;
	virtual void setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept override;
	virtual void getClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept override;
	virtual void setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept override;
	virtual void getMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept override;
	virtual void setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept override;
	virtual void getAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept override;
	virtual void setControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName, SetControlNameHandler const& handler) const noexcept override;
	virtual void getControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, GetControlNameHandler const& handler) const noexcept override;
	virtual void setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept override;
	virtual void getClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept override;
	virtual void setTimingName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& timingName, SetTimingNameHandler const& handler) const noexcept override;
	virtual void getTimingName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, GetTimingNameHandler const& handler) const noexcept override;
	virtual void setPtpInstanceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& ptpInstanceName, SetPtpInstanceNameHandler const& handler) const noexcept override;
	virtual void getPtpInstanceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, GetPtpInstanceNameHandler const& handler) const noexcept override;
	virtual void setPtpPortName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& ptpPortName, SetPtpPortNameHandler const& handler) const noexcept override;
	virtual void getPtpPortName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, GetPtpPortNameHandler const& handler) const noexcept override;
	virtual void setAssociation(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationHandler const& handler) const noexcept override;
	virtual void getAssociation(UniqueIdentifier const targetEntityID, GetAssociationHandler const& handler) const noexcept override;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept override;
	virtual void getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept override;
	virtual void setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const videoClusterIndex, entity::model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const sensorClusterIndex, entity::model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept override;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept override;
	virtual void getClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept override;
	virtual void setControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept override;
	virtual void getControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, GetControlValuesHandler const& handler) const noexcept override;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept override;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept override;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept override;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept override;
	virtual void getAvbInfo(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept override;
	virtual void getAsPath(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAsPathHandler const& handler) const noexcept override;
	virtual void getEntityCounters(UniqueIdentifier const targetEntityID, GetEntityCountersHandler const& handler) const noexcept override;
	virtual void getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept override;
	virtual void getClockDomainCounters(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept override;
	virtual void getStreamInputCounters(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept override;
	virtual void getStreamOutputCounters(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamOutputCountersHandler const& handler) const noexcept override;
	virtual void reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept override;
	virtual void rebootToFirmware(UniqueIdentifier const targetEntityID, entity::model::MemoryObjectIndex const memoryObjectIndex, RebootToFirmwareHandler const& handler) const noexcept override;
	virtual void startOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept override;
	virtual void abortOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept override;
	virtual void setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept override;
	virtual void getMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept override;
	virtual void getDynamicInfo(UniqueIdentifier const targetEntityID, entity::controller::DynamicInfoParameters const& parameters, GetDynamicInfoHandler const& handler) const noexcept override;
	virtual void setMaxTransitTime(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, std::chrono::nanoseconds const& maxTransitTime, SetMaxTransitTimeHandler const& handler) const noexcept override;
	virtual void getMaxTransitTime(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetMaxTransitTimeHandler const& handler) const noexcept override;
	virtual void addressAccess(UniqueIdentifier const targetEntityID, entity::addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept override;
	virtual void getMilanInfo(UniqueIdentifier const targetEntityID, GetMilanInfoHandler const& handler) const noexcept override;
	virtual void setSystemUniqueID(UniqueIdentifier const targetEntityID, UniqueIdentifier const systemUniqueID, entity::model::AvdeccFixedString const& systemName, SetSystemUniqueIDHandler const& handler) const noexcept override;
	virtual void getSystemUniqueID(UniqueIdentifier const targetEntityID, GetSystemUniqueIDHandler const& handler) const noexcept override;
	virtual void setMediaClockReferenceInfo(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, std::optional<entity::model::MediaClockReferencePriority> const userPriority, std::optional<entity::model::AvdeccFixedString> const& domainName, SetMediaClockReferenceInfoHandler const& handler) const noexcept override;
	virtual void getMediaClockReferenceInfo(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, GetMediaClockReferenceInfoHandler const& handler) const noexcept override;
	virtual void bindStream(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& talkerStream, entity::BindStreamFlags const flags, BindStreamHandler const& handler) const noexcept override;
	virtual void unbindStream(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, UnbindStreamHandler const& handler) const noexcept override;
	virtual void getStreamInputInfoEx(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputInfoExHandler const& handler) const noexcept override;
	virtual void connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept override;
	virtual void getTalkerStreamState(entity::model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept override;
	virtual void getTalkerStreamConnection(entity::model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept override;

	// Deleted compiler auto-generated methods
	ControllerVirtualProxy(ControllerVirtualProxy const&) = delete;
	ControllerVirtualProxy(ControllerVirtualProxy&&) = delete;
	ControllerVirtualProxy& operator=(ControllerVirtualProxy const&) = delete;
	ControllerVirtualProxy& operator=(ControllerVirtualProxy&&) = delete;

private:
	// Private methods
	/** Returns true if the specified UniqueIdentifier is a virtual entity */
	bool isVirtualEntity(UniqueIdentifier const& virtualEntity) const noexcept;

	/** Picks the most appropriate real interface for the specified entity based on current reachability. Falls back to primary if no reachability data is known. */
	entity::controller::Interface const* pickRealInterface(UniqueIdentifier const& targetEntityID) const noexcept;

	/** Routes an AEM/AA/MVU command through the dual-interface retry layer.
	 * @details Selects the appropriate real interface via pickRealInterface, wraps the handler with retry logic, and invokes the member function pointed to by @a Method.
	 * @tparam Method Pointer-to-member-function on entity::controller::Interface (the command to invoke).
	 * @tparam HandlerT Deduced type of the completion handler.
	 * @tparam Args Deduced types of the command arguments (excluding targetEntityID and handler).
	 * @param[in] targetEntityID Entity to target (used both as routing key and first method argument).
	 * @param[in] handler User-provided completion handler.
	 * @param[in] args Remaining arguments forwarded to the member function (between targetEntityID and handler).
	 */
	template<auto Method, typename HandlerT, typename... Args>
	void routeAemCommand(UniqueIdentifier const targetEntityID, HandlerT const& handler, Args const&... args) const noexcept;

	/** Routes an ACMP command through the dual-interface retry layer.
	 * @details Same as routeAemCommand but for ACMP commands where the routing key differs from the method arguments.
	 * @tparam Method Pointer-to-member-function on entity::controller::Interface (the ACMP command to invoke).
	 * @tparam HandlerT Deduced type of the completion handler.
	 * @tparam Args Deduced types of the command arguments (excluding handler).
	 * @param[in] routingKey EntityID used to pick the real interface via pickRealInterface.
	 * @param[in] handler User-provided completion handler.
	 * @param[in] args Arguments forwarded to the member function (before handler).
	 */
	template<auto Method, typename HandlerT, typename... Args>
	void routeAcmpCommand(UniqueIdentifier const routingKey, HandlerT const& handler, Args const&... args) const noexcept;

	// Private members
	mutable std::mutex _lock{};
	std::set<UniqueIdentifier> _virtualEntities{};
	protocol::ProtocolInterface const* _protocolInterface{ nullptr };
	entity::controller::Interface const* _realInterface{ nullptr }; /**< Primary real interface. */
	entity::controller::Interface const* _secondaryRealInterface{ nullptr }; /**< Secondary real interface, or nullptr in single-interface mode. */
	entity::controller::Interface const* _virtualInterface{ nullptr };
	std::string _executorName{};

	// Reachability tracking (dual-interface mode only)
	mutable std::mutex _reachabilityLock{};
	/** Per-(entity, PI) state combining reachability and unsol registration. */
	struct EntityState
	{
		bool onPrimary{ false };
		bool onSecondary{ false };
		UnsolState unsolPrimary{ UnsolState::NotRegistered };
		UnsolState unsolSecondary{ UnsolState::NotRegistered };
		std::set<entity::model::AvbInterfaceIndex> interfacesFromPrimary{}; /**< Interface indices last observed via Primary PI's ADP. */
		std::set<entity::model::AvbInterfaceIndex> interfacesFromSecondary{}; /**< Interface indices last observed via Secondary PI's ADP. */
	};
	std::unordered_map<UniqueIdentifier, EntityState, UniqueIdentifier::hash> _reachability{};
	bool _primaryInterfaceUp{ true }; /**< Per-PI transport-up flag for the primary interface (dual-interface mode only). */
	bool _secondaryInterfaceUp{ true }; /**< Per-PI transport-up flag for the secondary interface (dual-interface mode only). */
};

} // namespace controller
} // namespace avdecc
} // namespace la
