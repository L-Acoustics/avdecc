#pragma message("TODO: In (almost) each onXXXResult, check if configurationIndex is still the currentConfiguration. If not then stop the query. Maybe find a way to stop processing inflight queries too.")
#pragma message("TODO: For descriptor queries, do not store and remove in a std::set. Instead store a tuple<DescriptorKey, bool completed, resultHandlers>, so that we keep track of already completed queries. When a query is requested, first check if the descriptor has been retrieved")

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
* @file avdeccController.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/controller/avdeccController.hpp"
#include "la/avdecc/logger.hpp"
#include "avdeccControlledEntityImpl.hpp"
#include "config.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <cassert>

namespace la
{
namespace avdecc
{
namespace controller
{

bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept
{
	/* Here you have to choose a compatibility mode
	*  1/ Strict mode
	*     The version of the library used to compile must be strictly equivalent to the one used at runtime.
	*  2/ Backward compatibility mode
	*     A newer version of the library used at runtime is backward compatible with an older one used to compile.
	*     When using that mode, each class must use a virtual interface, and each new version must derivate from the interface to propose new methods.
	*  3/ A combination of the 2 above, using code to determine which one depending on the passed interfaceVersion value.
	*/

	// Interface versions should be strictly equivalent
	return InterfaceVersion == interfaceVersion;
}

std::string LA_AVDECC_CONTROLLER_CALL_CONVENTION getVersion() noexcept
{
	return std::string(LA_AVDECC_CONTROLLER_LIB_VERSION);
}

std::uint32_t LA_AVDECC_CONTROLLER_CALL_CONVENTION getInterfaceVersion() noexcept
{
	return InterfaceVersion;
}


/* ************************************************************ */
/* ControllerImpl class declaration                             */
/* ************************************************************ */
class ControllerImpl final : public Controller, private entity::ControllerEntity::Delegate
{
public:
	ControllerImpl(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale);

private:
	using OnlineControlledEntity = std::shared_ptr<ControlledEntityImpl>;

	virtual ~ControllerImpl() override;

	/* ************************************************************ */
	/* Controller overrides                                         */
	/* ************************************************************ */
	virtual void destroy() noexcept override;

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
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, std::vector<entity::model::AudioMapping> const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, std::vector<entity::model::AudioMapping> const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept override;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept override;

	virtual ControlledEntityGuard getControlledEntity(UniqueIdentifier const entityID) const noexcept override;

	virtual void lock() noexcept override;
	virtual void unlock() noexcept override;

	/* ************************************************************ */
	/* Result handlers                                              */
	/* ************************************************************ */
	/* Enumeration and Control Protocol (AECP) handlers */
	void onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept;
	void onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept;
	void onAudioUnitDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AudioUnitDescriptor const& descriptor) noexcept;
	void onStreamInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onStreamOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onAvbInterfaceDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex, entity::model::AvbInterfaceDescriptor const& descriptor) noexcept;
	void onClockSourceDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex, entity::model::ClockSourceDescriptor const& descriptor) noexcept;
	void onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept;
	void onStringsDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept;
	void onStreamPortInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept;
	void onStreamPortOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept;
	void onAudioClusterDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, entity::model::AudioClusterDescriptor const& descriptor) noexcept;
	void onAudioMapDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, entity::model::AudioMapDescriptor const& descriptor) noexcept;
	void onClockDomainDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainDescriptor const& descriptor) noexcept;
	void onGetStreamPortInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	void onGetStreamPortOutputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept;
	/* Connection Management Protocol (ACMP) handlers */
	void onConnectStreamResult(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onDisconnectStreamResult(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onGetListenerStreamStateResult(entity::ControllerEntity const* const controller, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept;

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
	virtual void onConnectStreamSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onFastConnectStreamSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onDisconnectStreamSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onGetListenerStreamStateSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
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

	/* ************************************************************ */
	/* Private methods used to update AEM and notify observers      */
	/* ************************************************************ */
	void updateAcquiredState(ControlledEntityImpl& entity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, bool const undefined = false) const;
	void updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const;
	void updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const;
	void updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const;
	void updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const;
	void updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const;
	void updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const;
	void updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const;
	void updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const;
	void updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const;
	void updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const;
	void updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const;
	void updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const;
	void updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const;
	void updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const;
	void updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const;
	void updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const;
	void updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const;
	void updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const;

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	void checkAdvertiseEntity(ControlledEntityImpl* const entity) const noexcept;
	bool checkRescheduleQuery(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	bool checkRescheduleQuery(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	bool checkRescheduleQuery(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept;
	void handleStreamStateNotification(ControlledEntityImpl* const listenerEntity, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIndex const listenerStreamIndex, bool const isConnected, entity::ConnectionFlags const flags, bool const isSniffed) const noexcept;
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
	std::unordered_map<UniqueIdentifier, OnlineControlledEntity> _controlledEntities;
	mutable std::mutex _lock{}; // A mutex to protect _controlledEntities
	EndStation::UniquePointer _endStation{ nullptr, nullptr };
	entity::ControllerEntity* _controller{ nullptr };
	std::string _preferedLocale{ "en-US" };
};


/* ************************************************************ */
/* ControllerImpl class definition                              */
/* ************************************************************ */
/* ************************************************************ */
/* Controller overrides                                         */
/* ************************************************************ */
ControllerImpl::ControllerImpl(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale)
	: _preferedLocale(preferedLocale)
{
	try
	{
		_endStation = EndStation::create(protocolInterfaceType, interfaceName);
		_controller = _endStation->addControllerEntity(progID, vendorEntityModelID, this);
	}
	catch (EndStation::Exception const& e)
	{
		auto const err = e.getError();
		switch (err)
		{
			case EndStation::Error::InvalidProtocolInterfaceType:
				throw Exception(Error::InvalidProtocolInterfaceType, e.what());
			case EndStation::Error::InterfaceOpenError:
				throw Exception(Error::InterfaceOpenError, e.what());
			case EndStation::Error::InterfaceNotFound:
				throw Exception(Error::InterfaceNotFound, e.what());
			case EndStation::Error::InterfaceInvalid:
				throw Exception(Error::InterfaceInvalid, e.what());
			default:
				AVDECC_ASSERT(false, "Unhandled exception");
				throw Exception(Error::InternalError, e.what());
		}
	}
	catch (Exception const& e)
	{
		AVDECC_ASSERT(false, "Unhandled exception");
		throw Exception(Error::InternalError, e.what());
	}
}

ControllerImpl::~ControllerImpl()
{
	// First, remove ourself from the controller's delegate, we don't want notifications anymore (even if one is coming before the end of the destructor, it's not a big deal, _controlledEntities will be empty)
	_controller->setDelegate(nullptr);

	decltype(_controlledEntities) controlledEntities;

	// Move all controlled Entities (under lock), we don't want them to be accessible during destructor
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);
		controlledEntities = std::move(_controlledEntities);
	}

	// Notify all entities they are going offline
	for (auto const& entityKV : controlledEntities)
	{
		auto const& entity = entityKV.second;
		if (entity->wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, this, entity.get());
		}
	}

	// Remove all observers, we don't want to trigger notifications for upcoming actions
	removeAllObservers();

	// Try to release all acquired entities by this controller before destroying everything
	for (auto const& entityKV : controlledEntities)
	{
		auto const& controlledEntity = entityKV.second;
		if (controlledEntity->isAcquired())
		{
			auto const& entityID = entityKV.first;
			_controller->releaseEntity(entityID, entity::model::DescriptorType::Entity, 0u, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!
		}
	}
}

void ControllerImpl::destroy() noexcept
{
	delete this;
}

/* Controller configuration */
void ControllerImpl::enableEntityAdvertising(std::uint32_t const availableDuration)
{
	if (!_controller->enableEntityAdvertising(availableDuration))
		throw Exception(Error::DuplicateProgID, "Specified ProgID already in use on the local computer");
}

void ControllerImpl::disableEntityAdvertising() noexcept
{
	_controller->disableEntityAdvertising();
}

/* Enumeration and Control Protocol (AECP) */
void ControllerImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity requested for ") + toHexString(targetEntityID, true));

		// Already acquired or acquiring, don't do anything (we want to try to acquire if it's flagged as acquired by another controller, in case it went offline without notice)
		if (controlledEntity->isAcquired() || controlledEntity->isAcquiring())
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity not sent, ") + toHexString(targetEntityID, true) + "is " + (controlledEntity->isAcquired() ? "already acquired" : "being acquired"));
			return;
		}
		controlledEntity->setAcquireState(model::AcquireState::TryAcquire);
		_controller->acquireEntity(targetEntityID, isPersistent, entity::model::DescriptorType::Entity, 0u, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				try
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::Success:
							updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
							break;
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
							updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
							break;
						case entity::ControllerEntity::AemCommandStatus::NotImplemented:
						case entity::ControllerEntity::AemCommandStatus::NotSupported:
							updateAcquiredState(*controlledEntity, getNullIdentifier(), descriptorType, descriptorIndex);
							break;
						default:
							// In case of error, set the state to undefined
							updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), descriptorType, descriptorIndex, true);
							break;
					}
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					AVDECC_ASSERT(false, "acquireEntity succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("acquireEntity succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, getNullIdentifier());
	}
}

void ControllerImpl::releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("releaseEntity requested for ") + toHexString(targetEntityID, true));
		_controller->releaseEntity(targetEntityID, entity::model::DescriptorType::Entity, 0u, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("releaseEntity result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				try
				{
					if (status == entity::ControllerEntity::AemCommandStatus::Success) // Only change the acquire state in case of success
						updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					AVDECC_ASSERT(false, "releaseEntity succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("releaseEntity succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, getNullIdentifier());
	}
}

void ControllerImpl::setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfiguration requested for ") + toHexString(targetEntityID, true));
		_controller->setConfiguration(targetEntityID, configurationIndex, [this, handler](entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfiguration result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateConfiguration(controller, *controlledEntity, configurationIndex);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setConfiguration succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setConfiguration succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputFormat requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamInputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputFormat result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateStreamInputFormat(*controlledEntity, streamIndex, streamFormat);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setStreamInputFormat succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamInputFormat succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputFormat requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputFormat result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateStreamOutputFormat(*controlledEntity, streamIndex, streamFormat);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setStreamOutputFormat succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamOutputFormat succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityName requested for ") + toHexString(targetEntityID, true));
		_controller->setEntityName(targetEntityID, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateEntityName(*controlledEntity, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setEntityName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setEntityName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityGroupName requested for ") + toHexString(targetEntityID, true));
		_controller->setEntityGroupName(targetEntityID, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityGroupName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateEntityGroupName(*controlledEntity, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setEntityGroupName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setEntityGroupName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfigurationName requested for ") + toHexString(targetEntityID, true));
		_controller->setConfigurationName(targetEntityID, configurationIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfigurationName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateConfigurationName(*controlledEntity, configurationIndex, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setConfigurationName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setConfigurationName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputName requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamInputName(targetEntityID, configurationIndex, streamIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateStreamInputName(*controlledEntity, configurationIndex, streamIndex, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setStreamInputName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamInputName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputName requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamOutputName(targetEntityID, configurationIndex, streamIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateStreamOutputName(*controlledEntity, configurationIndex, streamIndex, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setStreamOutputName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamOutputName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setAudioUnitSamplingRate requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(audioUnitIndex));
		_controller->setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setAudioUnitSamplingRate result for ") + toHexString(entityID, true) + ":" + std::to_string(audioUnitIndex) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the sampling rate in case of success
				{
					try
					{
						updateAudioUnitSamplingRate(*controlledEntity, audioUnitIndex, samplingRate);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setAudioUnitSamplingRate succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setAudioUnitSamplingRate succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setClockSource requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(clockDomainIndex));
		_controller->setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setClockSource result for ") + toHexString(entityID, true) + ":" + std::to_string(clockDomainIndex) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the clock source in case of success
				{
					try
					{
						updateClockSource(*controlledEntity, clockDomainIndex, clockSourceIndex);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "setClockSource succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setClockSource succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamInput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->startStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamInput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the running status in case of success
				{
					try
					{
						updateStreamInputRunningStatus(*controlledEntity, streamIndex, true);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "startStreamInput succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("startStreamInput succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamInput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->stopStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamInput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the running status in case of success
				{
					try
					{
						updateStreamInputRunningStatus(*controlledEntity, streamIndex, false);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "stopStreamInput succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("stopStreamInput succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamOutput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->startStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamOutput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the running status in case of success
				{
					try
					{
						updateStreamOutputRunningStatus(*controlledEntity, streamIndex, true);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "startStreamOutput succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("startStreamOutput succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamOutput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->stopStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamOutput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status) // Only change the running status in case of success
				{
					try
					{
						updateStreamOutputRunningStatus(*controlledEntity, streamIndex, false);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "stopStreamOutput succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("stopStreamOutput succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, std::vector<entity::model::AudioMapping> const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamInputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamInputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateStreamPortInputAudioMappingsAdded(*controlledEntity, streamPortIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "addStreamInputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("addStreamInputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamOutputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamOutputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateStreamPortOutputAudioMappingsAdded(*controlledEntity, streamPortIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "addStreamOutputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("addStreamOutputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, std::vector<entity::model::AudioMapping> const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamInputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamInputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateStreamPortInputAudioMappingsRemoved(*controlledEntity, streamPortIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "removeStreamInputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("removeStreamInputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamOutputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamOutputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				if (!!status)
				{
					try
					{
						updateStreamPortOutputAudioMappingsRemoved(*controlledEntity, streamPortIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						AVDECC_ASSERT(false, "removeStreamOutputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("removeStreamOutputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::connectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("connectStream requested for ") + toHexString(listenerEntityID, true));
		_controller->connectStream(talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("connectStream result for ") + toHexString(listenerEntityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto listener = getControlledEntityImpl(listenerEntityID);
			auto talker = getControlledEntityImpl(talkerEntityID);
			entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

			if (listener)
			{
				controlStatus = status;
				if (!!status)
				{
					// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
					handleStreamStateNotification(listener.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, true, flags, false);
				}
			}
			invokeProtectedHandler(handler, talker.get(), listener.get(), talkerStreamIndex, listenerStreamIndex, controlStatus);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::disconnectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream requested for ") + toHexString(listenerEntityID, true));
		_controller->disconnectStream(talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*talkerEntityID*/, entity::model::StreamIndex const /*talkerStreamIndex*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream result for ") + toHexString(listenerEntityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto listener = getControlledEntityImpl(listenerEntityID);
			entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };
			bool shouldNotifyHandler{ true }; // Shall we notify the handler right now, or do we have to send another message before

			if (listener)
			{
				controlStatus = status;
				// In case of a disconnect we might get an error (forwarded from the talker) but the stream is actually disconnected.
				// In that case, we have to query the listener stream state in order to know the actual connection state
				if (!status && status != entity::ControllerEntity::ControlStatus::NotConnected)
				{
					shouldNotifyHandler = false; // Don't notify handler right now, wait for getListenerStreamState answer
					_controller->getListenerStreamState(listenerEntityID, listenerStreamIndex, [this, handler, disconnectStatus = status](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
					{
						// Take a copy of the ControlledEntity so we don't have to keep the lock
						auto listener = getControlledEntityImpl(listenerEntityID);
						entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

						if (listener)
						{
							if (!!status)
							{
								// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
								bool const isStillConnected = connectionCount != 0;
								handleStreamStateNotification(listener.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, isStillConnected, flags, false);
								// Status to return depends if we actually got disconnected (success in that case)
								controlStatus = isStillConnected ? disconnectStatus : entity::ControllerEntity::ControlStatus::Success;
							}
						}
						invokeProtectedHandler(handler, listener.get(), listenerStreamIndex, controlStatus);
					});
				}
				else if (!!status) // No error, update the connection state
				{
					// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
					handleStreamStateNotification(listener.get(), getNullIdentifier(), 0, listenerStreamIndex, false, flags, false);
				}
			}
			if (shouldNotifyHandler)
				invokeProtectedHandler(handler, listener.get(), listenerStreamIndex, controlStatus);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::getListenerStreamState(UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream requested for ") + toHexString(listenerEntityID, true));
		_controller->getListenerStreamState(listenerEntityID, listenerStreamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream result for ") + toHexString(listenerEntityID, true) + ": " + entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto listener = getControlledEntityImpl(listenerEntityID);
			auto talker = getControlledEntityImpl(talkerEntityID);
			entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

			if (listener)
			{
				controlStatus = status;
				if (!!status)
				{
					// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
					handleStreamStateNotification(listener.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, connectionCount != 0, flags, false);
					checkAdvertiseEntity(listener.get());
				}
			}
			invokeProtectedHandler(handler, talker.get(), listener.get(), talkerStreamIndex, listenerStreamIndex, connectionCount, flags, controlStatus);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), uint16_t(0), entity::ConnectionFlags::None, entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

ControlledEntityGuard ControllerImpl::getControlledEntity(UniqueIdentifier const entityID) const noexcept
{
	return ControlledEntityGuard{ getControlledEntityImpl(entityID) };
}

void ControllerImpl::lock() noexcept
{
	_controller->lock();
}

void ControllerImpl::unlock() noexcept
{
	_controller->unlock();
}

/* ************************************************************ */
/* Result handlers                                              */
/* ************************************************************ */
/* Enumeration and Control Protocol (AECP) handlers */
void ControllerImpl::onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityDescriptorResult(") + toHexString(entityID, true) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Entity, 0) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setEntityDescriptor(descriptor);
					for (auto index = entity::model::ConfigurationIndex(0u); index < descriptor.configurationsCount; ++index)
					{
						controlledEntity->setDescriptorExpected(0, entity::model::DescriptorType::Configuration, index);
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readConfigurationDescriptor(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
						controller->readConfigurationDescriptor(entityID, index, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), 0, entity::model::DescriptorType::Entity, 0))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConfigurationDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Configuration, configurationIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setConfigurationDescriptor(descriptor, configurationIndex);
					auto const& entityDescriptor = controlledEntity->getEntityStaticModel().entityDescriptor;
					auto const isCurrentConfiguration = configurationIndex == entityDescriptor.currentConfiguration;
					// Only get full descriptors for active configuration
					if (isCurrentConfiguration)
					{
						// Get Locales
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::LocaleIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::Locale, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readLocaleDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readLocaleDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get audio units
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::AudioUnit);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::AudioUnitIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioUnit, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioUnitDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readAudioUnitDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onAudioUnitDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get input streams
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamInput);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::StreamIndex(0); index < count; ++index)
								{
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getListenerStreamState(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
									controller->getListenerStreamState(entityID, index, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, configurationIndex));

									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamInput, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamInputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readStreamInputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get output streams
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamOutput);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::StreamIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamOutput, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamOutputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readStreamOutputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get avb interfaces
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::AvbInterface);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AvbInterface, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAvbInterfaceDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readAvbInterfaceDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onAvbInterfaceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get clock sources
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::ClockSource);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::ClockSourceIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::ClockSource, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readClockSourceDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readClockSourceDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get clock domains
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::ClockDomain);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::ClockDomainIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::ClockDomain, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readClockDomainDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readClockDomainDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onClockDomainDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
					}
					// For non-active configurations, just get locales (and strings)
					else
					{
						// Get Locales
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::LocaleIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::Locale, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readLocaleDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readLocaleDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), 0, entity::model::DescriptorType::Configuration, configurationIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAudioUnitDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AudioUnitDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAudioUnitDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(audioUnitIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAudioUnitDescriptor(descriptor, configurationIndex, audioUnitIndex);

					// Get stream port input
					{
						if (descriptor.numberOfStreamInputPorts != 0)
						{
							for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamInputPorts; ++index)
							{
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamPortInput, index);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamPortInputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
								controller->readStreamPortInputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamPortInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					// Get stream port output
					{
						if (descriptor.numberOfStreamOutputPorts != 0)
						{
							for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamOutputPorts; ++index)
							{
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamPortOutput, index);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamPortOutputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
								controller->readStreamPortOutputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamPortOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamInputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamInputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamInputDescriptor(descriptor, configurationIndex, streamIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamOutputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamOutputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamOutputDescriptor(descriptor, configurationIndex, streamIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAvbInterfaceDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex, entity::model::AvbInterfaceDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAvbInterfaceDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(interfaceIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAvbInterfaceDescriptor(descriptor, configurationIndex, interfaceIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onClockSourceDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex, entity::model::ClockSourceDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onClockSourceDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clockIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setClockSourceDescriptor(descriptor, configurationIndex, clockIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onLocaleDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(localeIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Locale, localeIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setLocaleDescriptor(descriptor, configurationIndex, localeIndex);
					auto const& configStaticModel = controlledEntity->getConfigurationStaticModel(configurationIndex);
					std::uint16_t countLocales{ 0u };
					{
						auto const localeIt = configStaticModel.configurationDescriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
						if (localeIt != configStaticModel.configurationDescriptor.descriptorCounts.end())
							countLocales = localeIt->second;
					}
					auto const allLocalesLoaded = configStaticModel.localeDescriptors.size() == countLocales;
					// We got all locales, now load strings for the desired locale
					if (allLocalesLoaded)
					{
						entity::model::LocaleDescriptor const* localeDescriptor{ nullptr };
						localeDescriptor = controlledEntity->findLocaleDescriptor(configurationIndex, _preferedLocale);
						if (localeDescriptor == nullptr)
						{
#pragma message("TODO: Split _preferedLocale into language/country, then if findLocaleDescriptor fails and language is not 'en', try to find a locale for 'en'")
							localeDescriptor = controlledEntity->findLocaleDescriptor(configurationIndex, "en");
						}
						if (localeDescriptor != nullptr)
						{
							controlledEntity->setSelectedLocaleDescriptor(configurationIndex, localeDescriptor);
							for (auto index = entity::model::StringsIndex(0); index < localeDescriptor->numberOfStringDescriptors; ++index)
							{
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::Strings, index);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStringsDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(localeDescriptor->baseStringDescriptorIndex + index) + ")");
								controller->readStringsDescriptor(entityID, configurationIndex, localeDescriptor->baseStringDescriptorIndex + index, std::bind(&ControllerImpl::onStringsDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, localeDescriptor->baseStringDescriptorIndex));
							}
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Locale, localeIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStringsDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStringsDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(stringsIndex) + "," + std::to_string(baseStringDescriptorIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Strings, stringsIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->addStringsDescriptor(descriptor, configurationIndex, stringsIndex, baseStringDescriptorIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StringsDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Strings, stringsIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StringsDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamPortInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamPortInputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamPortIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamPortInputDescriptor(descriptor, configurationIndex, streamPortIndex);
					// Get audio clusters
					{
						if (descriptor.numberOfClusters != 0)
						{
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < descriptor.numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + descriptor.baseCluster);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioClusterDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clusterIndex) + ")");
								controller->readAudioClusterDescriptor(entityID, configurationIndex, clusterIndex, std::bind(&ControllerImpl::onAudioClusterDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					// Get audio maps (static or dynamic)
					{
						if (descriptor.numberOfMaps != 0)
						{
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioMapDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(mapIndex) + ")");
								controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
						else
						{
							// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
							controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
							Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortInputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
							controller->getStreamPortInputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortInputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortInputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamPortOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamPortOutputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamPortIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamPortOutputDescriptor(descriptor, configurationIndex, streamPortIndex);
					// Get audio clusters
					{
						if (descriptor.numberOfClusters != 0)
						{
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < descriptor.numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + descriptor.baseCluster);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioClusterDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clusterIndex) + ")");
								controller->readAudioClusterDescriptor(entityID, configurationIndex, clusterIndex, std::bind(&ControllerImpl::onAudioClusterDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					// Get audio maps (static or dynamic)
					{
						if (descriptor.numberOfMaps != 0)
						{
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioMapDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(mapIndex) + ")");
								controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
						else
						{
							// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
							controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
							Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortOutputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
							controller->getStreamPortOutputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortOutputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortOutputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAudioClusterDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, entity::model::AudioClusterDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAudioClusterDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clusterIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAudioClusterDescriptor(descriptor, configurationIndex, clusterIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAudioMapDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, entity::model::AudioMapDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAudioMapDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAudioMapDescriptor(descriptor, configurationIndex, mapIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioMapDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioMapDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onClockDomainDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onClockDomainDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clockDomainIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setClockDomainDescriptor(descriptor, configurationIndex, clockDomainIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onGetStreamPortInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamPortInputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					if (mapIndex == 0)
						controlledEntity->clearPortInputStreamAudioMappings(configurationIndex, streamPortIndex);
					bool isComplete = mapIndex == (numberOfMaps - 1);
					controlledEntity->addPortInputStreamAudioMappings(configurationIndex, streamPortIndex, mappings);
					if (!isComplete)
					{
						controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortInputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
						controller->getStreamPortInputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(mapIndex + 1), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
					}
					else
					{
						checkAdvertiseEntity(controlledEntity.get());
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
				}
			}
			else
			{
#if IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
				if (status == entity::ControllerEntity::AemCommandStatus::NotImplemented || status == entity::ControllerEntity::AemCommandStatus::NotSupported)
				{
					checkAdvertiseEntity(controlledEntity.get());
					return;
				}
#endif
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
				}
			}
		}
	}
}

void ControllerImpl::onGetStreamPortOutputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamPortOutputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					if (mapIndex == 0)
						controlledEntity->clearPortOutputStreamAudioMappings(configurationIndex, streamPortIndex);
					bool isComplete = mapIndex == (numberOfMaps - 1);
					controlledEntity->addPortOutputStreamAudioMappings(configurationIndex, streamPortIndex, mappings);
					if (!isComplete)
					{
						controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortOutputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
						controller->getStreamPortOutputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(mapIndex + 1), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
					}
					else
					{
						checkAdvertiseEntity(controlledEntity.get());
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
				}
			}
			else
			{
#if IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
				if (status == entity::ControllerEntity::AemCommandStatus::NotImplemented || status == entity::ControllerEntity::AemCommandStatus::NotSupported)
				{
					checkAdvertiseEntity(controlledEntity.get());
					return;
				}
#endif
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
				}
			}
		}
	}
}

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConnectStreamResult(") + toHexString(talkerEntityID, true) + "," + std::to_string(talkerStreamIndex) + "," + toHexString(listenerEntityID, true) + "," + std::to_string(listenerStreamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + entity::ControllerEntity::statusToString(status) + ")");
}

void ControllerImpl::onDisconnectStreamResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onDisconnectStreamResult(") + toHexString(talkerEntityID, true) + "," + std::to_string(talkerStreamIndex) + "," + toHexString(listenerEntityID, true) + "," + std::to_string(listenerStreamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + entity::ControllerEntity::statusToString(status) + ")");
}

void ControllerImpl::onGetListenerStreamStateResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetListenerStreamStateResult(") + toHexString(listenerEntityID, true) + "," + entity::ControllerEntity::statusToString(status) + ")");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto listener = getControlledEntityImpl(listenerEntityID);

	if (listener)
	{
		if (listener->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStreamIndex) && !listener->gotEnumerationError())
		{
			if (!!status)
			{
				// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
				handleStreamStateNotification(listener.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, connectionCount != 0, flags, false);
				checkAdvertiseEntity(listener.get());
			}
			else
			{
				if (!checkRescheduleQuery(status, listener.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStreamIndex))
				{
					listener->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, listener.get(), QueryCommandError::ListenerStreamState);
				}
			}
		}
	}
}

/* ************************************************************ */
/* entity::ControllerEntity::Delegate overrides                 */
/* ************************************************************ */
/* Global notifications */
void ControllerImpl::onTransportError(entity::ControllerEntity const* const /*controller*/) noexcept
{
	notifyObserversMethod<Controller::Observer>(&Controller::Observer::onTransportError, this);
}

/* Discovery Protocol (ADP) delegate */
void ControllerImpl::onEntityOnline(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityOnline: ") + toHexString(entityID, true));

	OnlineControlledEntity controlledEntity{};

	// Create and add the entity
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

#ifdef DEBUG
		AVDECC_ASSERT(_controlledEntities.find(entityID) == _controlledEntities.end(), "Entity already online");
#endif

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt == _controlledEntities.end())
		{
			controlledEntity = _controlledEntities.insert(std::make_pair(entityID, std::make_shared<ControlledEntityImpl>(entity))).first->second;
		}
	}

	if (controlledEntity)
	{
		// Check if AEM is supported by this entity
		if (hasFlag(entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		{
			// Register for unsolicited notifications
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Registering for unsolicited notifications"));
			controller->registerUnsolicitedNotifications(entityID, {});

			// Request its entity descriptor
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Requesting entity's descriptor"));
			controlledEntity->setDescriptorExpected(0, entity::model::DescriptorType::Entity, 0);
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readEntityDescriptor(") + toHexString(entityID, true) + ")");
			controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		else
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Entity does not support AEM, simply advertise it"));
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
	else
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Debug, std::string("onEntityOnline: Entity already registered, updating it"));
		// This should not happen, but just in case... update it
		onEntityUpdate(controller, entityID, entity);
	}

}

void ControllerImpl::onEntityUpdate(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityUpdate: ") + toHexString(entityID, true));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

#ifdef DEBUG
	AVDECC_ASSERT(controlledEntity, "Unknown entity");
#endif
	if (controlledEntity)
	{
		controlledEntity->updateEntity(entity);
		// Maybe we need to notify the upper layers (someday, if required)
	}
}

void ControllerImpl::onEntityOffline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityOffline: ") + toHexString(entityID, true));

	OnlineControlledEntity controlledEntity{};

	// Cleanup and remove the entity
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt != _controlledEntities.end())
		{
			controlledEntity = entityIt->second;
			_controlledEntities.erase(entityIt);
		}
	}

	if (controlledEntity)
	{
		updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), entity::model::DescriptorType::Entity, 0u, true);

		// Entity was advertised to the user, notify observers
		if (controlledEntity->wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, this, controlledEntity.get());
			controlledEntity->setAdvertised(false);
		}
	}
}

/* Connection Management Protocol sniffed messages (ACMP) */
void ControllerImpl::onConnectStreamSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
			handleStreamStateNotification(controlledEntity.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, true, flags, true);
		}
		// We don't care about sniffed errors
	}
}

void ControllerImpl::onFastConnectStreamSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
			handleStreamStateNotification(controlledEntity.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, true, flags, true);
		}
		// We don't care about sniffed errors
	}
}

void ControllerImpl::onDisconnectStreamSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*talkerEntityID*/, entity::model::StreamIndex const /*talkerStreamIndex*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
			handleStreamStateNotification(controlledEntity.get(), getNullIdentifier(), 0, listenerStreamIndex, false, flags, true);
		}
		// We don't care about sniffed errors
	}
}

void ControllerImpl::onGetListenerStreamStateSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerEntityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
			handleStreamStateNotification(controlledEntity.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, connectionCount != 0, flags, true);
			checkAdvertiseEntity(controlledEntity.get());
		}
		// We don't care about sniffed errors
	}
}

/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
void ControllerImpl::onEntityAcquired(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onEntityAcquired succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityAcquired(" + toHexString(entityID, true) + "," + toHexString(owningEntity, true) + "," + std::to_string(to_integral(descriptorType)) + "," + std::to_string(descriptorIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onEntityReleased(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onEntityReleased succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityReleased(" + toHexString(entityID, true) + "," + toHexString(owningEntity, true) + "," + std::to_string(to_integral(descriptorType)) + "," + std::to_string(descriptorIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onConfigurationChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateConfiguration(controller, *controlledEntity, configurationIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onConfigurationChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onConfigurationChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamInputFormat(*controlledEntity, streamIndex, streamFormat);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamInputFormatChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputFormatChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + toHexString(streamFormat, true) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamOutputFormat(*controlledEntity, streamIndex, streamFormat);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamOutputFormatChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputFormatChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + toHexString(streamFormat, true) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamPortInputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			// Only support the case where numberOfMaps == 1
			if (numberOfMaps != 1 || mapIndex != 0)
				return;

			auto const& entityDescriptor = controlledEntity->getEntityStaticModel().entityDescriptor;
			controlledEntity->clearPortInputStreamAudioMappings(entityDescriptor.currentConfiguration, streamPortIndex);
			updateStreamPortInputAudioMappingsAdded(*controlledEntity, streamPortIndex, mappings);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamInputAudioMappingsChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamPortInputAudioMappingsChanged(" + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamPortOutputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			// Only support the case where numberOfMaps == 1
			if (numberOfMaps != 1 || mapIndex != 0)
				return;

			auto const& entityDescriptor = controlledEntity->getEntityStaticModel().entityDescriptor;
			controlledEntity->clearPortOutputStreamAudioMappings(entityDescriptor.currentConfiguration, streamPortIndex);
			updateStreamPortOutputAudioMappingsAdded(*controlledEntity, streamPortIndex, mappings);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamOutputAudioMappingsChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamPortOutputAudioMappingsChanged(" + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamInputInfo(*controlledEntity, streamIndex, info);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamInputInfoChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputInfoChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamOutputInfo(*controlledEntity, streamIndex, info);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamOutputInfoChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputInfoChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onEntityNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateEntityName(*controlledEntity, entityName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onEntityNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityNameChanged(" + toHexString(entityID, true) + "," + std::string(entityName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onEntityGroupNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateEntityGroupName(*controlledEntity, entityGroupName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onEntityGroupNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityGroupNameChanged(" + toHexString(entityID, true) + "," + std::string(entityGroupName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onConfigurationNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateConfigurationName(*controlledEntity, configurationIndex, configurationName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onConfigurationNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onConfigurationNameChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex ) + "," + std::string(configurationName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamInputName(*controlledEntity, configurationIndex, streamIndex, streamName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamInputNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputNameChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex ) + "," + std::string(streamName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamOutputName(*controlledEntity, configurationIndex, streamIndex, streamName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamOutputNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputNameChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + std::string(streamName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onAudioUnitSamplingRateChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateAudioUnitSamplingRate(*controlledEntity, audioUnitIndex, samplingRate);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onAudioUnitSamplingRateChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onAudioUnitSamplingRateChanged(" + toHexString(entityID, true) + "," + std::to_string(audioUnitIndex) + "," + std::to_string(samplingRate) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onClockSourceChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateClockSource(*controlledEntity, clockDomainIndex, clockSourceIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onClockSourceChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onClockSourceChanged(" + toHexString(entityID, true) + "," + std::to_string(clockDomainIndex) + "," + std::to_string(clockSourceIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamInputRunningStatus(*controlledEntity, streamIndex, true);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamInputStarted succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputStarted(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamOutputRunningStatus(*controlledEntity, streamIndex, true);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamOutputStarted succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputStarted(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamInputRunningStatus(*controlledEntity, streamIndex, false);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamInputStarted succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputStarted(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		try
		{
			updateStreamOutputRunningStatus(*controlledEntity, streamIndex, false);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			AVDECC_ASSERT(false, "onStreamOutputStarted succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputStarted(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + ") failed: " + e.what()));
		}
	}
}

/* ************************************************************ */
/* Private methods used to update AEM and notify observers      */
/* ************************************************************ */
void ControllerImpl::updateAcquiredState(ControlledEntityImpl& controlledEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/, bool const undefined) const
{
#pragma message("TODO: Handle acquire state tree")
	if (descriptorType == entity::model::DescriptorType::Entity)
	{
		auto owningController{ getUninitializedIdentifier() };
		auto acquireState{ model::AcquireState::Undefined };
		if (undefined)
		{
			owningController = getUninitializedIdentifier();
			acquireState = model::AcquireState::Undefined;
		}
		else
		{
			owningController = owningEntity;

			if (!isValidUniqueIdentifier(owningEntity)) // No more controller
				acquireState = model::AcquireState::NotAcquired;
			else if (owningEntity == _controller->getEntityID()) // Controlled by myself
				acquireState = model::AcquireState::Acquired;
			else // Or acquired by another controller
				acquireState = model::AcquireState::AcquiredByOther;
		}

		controlledEntity.setAcquireState(acquireState);
		controlledEntity.setOwningController(owningController);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAcquireStateChanged, this, &controlledEntity, acquireState, owningController);
		}
	}
}

void ControllerImpl::updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;

		entityDescriptor.currentConfiguration = configurationIndex;

		// Right now, simulate the entity going offline then online again - TODO: Handle multiple configurations, see https://github.com/L-Acoustics/avdecc/issues/3
		auto const e = controlledEntity.getEntity(); // Make a copy of the Entity object since it will be destroyed during onEntityOffline
		auto const entityID = e.getEntityID();
		auto* self = const_cast<ControllerImpl*>(this);
		self->onEntityOffline(controller, entityID);
		self->onEntityOnline(controller, entityID, e);
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(entityDescriptor.currentConfiguration, streamIndex);
		
		streamDescriptor.currentFormat = streamFormat;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(entityDescriptor.currentConfiguration, streamIndex);

		streamDescriptor.currentFormat = streamFormat;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& streamDynamicModel = controlledEntity.getStreamInputDynamicModel(entityDescriptor.currentConfiguration, streamIndex);

		streamDynamicModel.streamInfo = info;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputInfoChanged, this, &controlledEntity, streamIndex, info);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& streamDynamicModel = controlledEntity.getStreamOutputDynamicModel(entityDescriptor.currentConfiguration, streamIndex);

		streamDynamicModel.streamInfo = info;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputInfoChanged, this, &controlledEntity, streamIndex, info);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;

		entityDescriptor.entityName = entityName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityNameChanged, this, &controlledEntity, entityName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;

		entityDescriptor.groupName = entityGroupName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityGroupNameChanged, this, &controlledEntity, entityGroupName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const
{
	try
	{
		auto& configurationDescriptor = controlledEntity.getConfigurationStaticModel(configurationIndex).configurationDescriptor;

		configurationDescriptor.objectName = configurationName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConfigurationNameChanged, this, &controlledEntity, configurationIndex, configurationName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}
	
void ControllerImpl::updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;

#pragma message("TODO: Handle multiple configurations, not just the active one (no need to use getConfigurationNode, getStreamInputNode shall be used directly)")
		if (configurationIndex == entityDescriptor.currentConfiguration)
		{
			auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(configurationIndex, streamIndex);
			streamDescriptor.objectName = streamInputName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamInputName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;

#pragma message("TODO: Handle multiple configurations, not just the active one (no need to use getConfigurationNode, getStreamOutputNode shall be used directly)")
		if (configurationIndex == entityDescriptor.currentConfiguration)
		{
			auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(configurationIndex, streamIndex);
			streamDescriptor.objectName = streamOutputName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamOutputName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& audioUnitDescriptor = controlledEntity.getAudioUnitDescriptor(entityDescriptor.currentConfiguration, audioUnitIndex);

		audioUnitDescriptor.currentSamplingRate = samplingRate;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitSamplingRateChanged, this, &controlledEntity, audioUnitIndex, samplingRate);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& domainDescriptor = controlledEntity.getClockDomainDescriptor(entityDescriptor.currentConfiguration, clockDomainIndex);

		domainDescriptor.clockSourceIndex = clockSourceIndex;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceChanged, this, &controlledEntity, clockDomainIndex, clockSourceIndex);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& streamDynamicModel = controlledEntity.getStreamInputDynamicModel(entityDescriptor.currentConfiguration, streamIndex);

		streamDynamicModel.isRunning = isRunning;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			if (isRunning)
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStarted, this, &controlledEntity, streamIndex);
			else
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStopped, this, &controlledEntity, streamIndex);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
		auto& streamDynamicModel = controlledEntity.getStreamOutputDynamicModel(entityDescriptor.currentConfiguration, streamIndex);

		streamDynamicModel.isRunning = isRunning;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			if (isRunning)
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStarted, this, &controlledEntity, streamIndex);
			else
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStopped, this, &controlledEntity, streamIndex);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
	controlledEntity.addPortInputStreamAudioMappings(entityDescriptor.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
	controlledEntity.removePortInputStreamAudioMappings(entityDescriptor.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
	controlledEntity.addPortOutputStreamAudioMappings(entityDescriptor.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityStaticModel().entityDescriptor;
	controlledEntity.removePortOutputStreamAudioMappings(entityDescriptor.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
void ControllerImpl::checkAdvertiseEntity(ControlledEntityImpl* const entity) const noexcept
{
	if (!entity->wasAdvertised())
	{
		if (entity->gotAllExpectedDescriptors() && entity->gotAllExpectedDynamicInfo())
		{
			entity->setAdvertised(true);
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOnline, this, entity);
		}
	}
}

bool ControllerImpl::checkRescheduleQuery(entity::ControllerEntity::AemCommandStatus const /*status*/, ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, entity::model::DescriptorType const /*descriptorType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query") // We might want to have a map or micro-methods to run the query based on the parameters (and first-time queries shall use them too)
	return false;
}

bool ControllerImpl::checkRescheduleQuery(entity::ControllerEntity::AemCommandStatus const /*status*/, ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DynamicInfoType const /*dynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query")
	return false;
}

bool ControllerImpl::checkRescheduleQuery(entity::ControllerEntity::ControlStatus const /*status*/, ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DynamicInfoType const /*dynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query")
	return false;
}

/* _lockEntities must be taken */
void ControllerImpl::handleStreamStateNotification(ControlledEntityImpl* const listenerEntity, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIndex const listenerStreamIndex, bool const isConnected, entity::ConnectionFlags const flags, bool const isSniffed) const noexcept
{
	try
	{
		// Convert talker EID to Null EID if it's the Uninitialized one - We prefer to store a not-connected listener using NullIdentifier
		auto talker = talkerEntityID;
		if (talkerEntityID == getUninitializedIdentifier())
			talker = getNullIdentifier();

		// Build a StreamConnectedState
		auto const state = entity::model::StreamConnectedState{ talker, talkerStreamIndex, isConnected, avdecc::hasFlag(flags, entity::ConnectionFlags::FastConnect) };
		auto const cachedState = listenerEntity->getConnectedSinkState(listenerStreamIndex);

		// Check the previous state, and detect if it changed
		if (state != cachedState)
		{
			// Update our internal cache
			auto const& entityDescriptor = listenerEntity->getEntityStaticModel().entityDescriptor;
			listenerEntity->setInputStreamState(state, entityDescriptor.currentConfiguration, listenerStreamIndex);
			// Switching from a connected stream to another connected stream *might* happen, in case we missed the Disconnect
			// Notify observers if the stream state was sniffed (and listener was advertised)
			if (isSniffed && listenerEntity->wasAdvertised())
			{
				// Got disconnected
				if (!state.isConnected && cachedState.isConnected)
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDisconnectStreamSniffed, this, listenerEntity, listenerStreamIndex);
				}
				// New connection
				else if (state.isConnected && !cachedState.isConnected)
				{
					ControlledEntityImpl* talkerEntity{ nullptr };
					auto talkerEntityIt = _controlledEntities.find(talker);
					if (talkerEntityIt != _controlledEntities.end())
						talkerEntity = talkerEntityIt->second.get();

					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConnectStreamSniffed, this, talkerEntity, listenerEntity, talkerStreamIndex, listenerStreamIndex);
				}
				// We missed a Disconnect, simulate it
				else if (state.isConnected && cachedState.isConnected)
				{
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Debug, std::string("Switching from a connected stream to another, we missed a Disconnect message!"));

					// Simulate the disconnect
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDisconnectStreamSniffed, this, listenerEntity, listenerStreamIndex);

					// And notify the new connection
					ControlledEntityImpl* talkerEntity{ nullptr };
					auto talkerEntityIt = _controlledEntities.find(talker);
					if (talkerEntityIt != _controlledEntities.end())
						talkerEntity = talkerEntityIt->second.get();

					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConnectStreamSniffed, this, talkerEntity, listenerEntity, talkerStreamIndex, listenerStreamIndex);
				}
			}
		}
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions from getConnectedSink
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
	}
}

/* ************************************************************ */
/* Controller class definition                                  */
/* ************************************************************ */
Controller* LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::createRawController(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale)
{
	return new ControllerImpl(protocolInterfaceType, interfaceName, progID, vendorEntityModelID, preferedLocale);
}

} // namespace controller
} // namespace avdecc
} // namespace la
