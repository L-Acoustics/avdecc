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
* @file avdeccController.hpp
* @author Christophe Calmejane
* @brief Avdecc controller.
*/

#pragma once

#include <memory>
#include <functional>
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/internals/exception.hpp>
#include "internals/avdeccControlledEntity.hpp"
#include "internals/exports.hpp"

namespace la
{
namespace avdecc
{
namespace controller
{

/**
* Interface version of the library, used to check for compatibility between the version used to compile and the runtime version.<BR>
* Everytime the interface changes (what is visible from the user) you increase the InterfaceVersion value.<BR>
* A change in the visible interface is any modification in a public header file except a change in a private non-virtual method
* (either added, removed or signature modification).
* Any other change (including templates, inline methods, defines, typedefs, ...) are considered a modification of the interface.
*/
constexpr std::uint32_t InterfaceVersion = 205;

/**
* @brief Checks if the library is compatible with specified interface version.
* @param[in] interfaceVersion The interface version to check for compatibility.
* @return True if the library is compatible.
* @note If the library is not compatible, the application should no longer use the library.<BR>
*       When using the avdecc controller shared library, you must call this version to check the compatibility between the compiled and the loaded version.
*/
LA_AVDECC_CONTROLLER_API bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept;

/**
* @brief Gets the avdecc controller library version.
* @details Returns the avdecc controller library version.
* @return A string representing the library version.
*/
LA_AVDECC_CONTROLLER_API std::string LA_AVDECC_CONTROLLER_CALL_CONVENTION getVersion() noexcept;

/**
* @brief Gets the avdecc controller shared library interface version.
* @details Returns the avdecc controller shared library interface version.
* @return The interface version.
*/
LA_AVDECC_CONTROLLER_API std::uint32_t LA_AVDECC_CONTROLLER_CALL_CONVENTION getInterfaceVersion() noexcept;

enum class CompileOption : std::uint32_t
{
	None = 0,
	IgnoreNeitherStaticNorDynamicMappings = 1u << 0,
	EnableRedundancy = 1u << 15,
	Strict2018Redundancy = 1u << 16,
};
using CompileOptions = CompileOption;

struct CompileOptionInfo
{
	CompileOption option{ CompileOption::None };
	std::string shortName{};
	std::string longName{};
};

/**
* @brief Gets the avdecc controller library compile options.
* @details Returns the avdecc controller library compile options.
* @return The compile options.
*/
LA_AVDECC_CONTROLLER_API CompileOptions LA_AVDECC_CONTROLLER_CALL_CONVENTION getCompileOptions() noexcept;

/**
* @brief Gets the avdecc controller library compile options info.
* @details Returns the avdecc controller library compile options info.
* @return The compile options info.
*/
LA_AVDECC_CONTROLLER_API std::vector<CompileOptionInfo> LA_AVDECC_CONTROLLER_CALL_CONVENTION getCompileOptionsInfo() noexcept;

/* ************************************************************************** */
/* Controller                                                                 */
/* ************************************************************************** */
class Controller : public la::avdecc::Subject<Controller, std::recursive_mutex>
{
public:
	using UniquePointer = std::unique_ptr<Controller, void(*)(Controller*)>;

	enum class Error
	{
		NoError = 0,
		InvalidProtocolInterfaceType = 1, /**< Selected protocol interface type is invalid */
		InterfaceOpenError = 2, /**< Failed to open interface. */
		InterfaceNotFound = 3, /**< Specified interface not found. */
		InterfaceInvalid = 4, /**< Specified interface is invalid. */
		DuplicateProgID = 5, /**< Specified ProgID is already in use on the local computer. */
		InternalError = 99, /**< Internal error, please report the issue. */
	};

	class LA_AVDECC_CONTROLLER_API Exception final : public la::avdecc::Exception
	{
	public:
		template<class T>
		Exception(Error const error, T&& text) noexcept : la::avdecc::Exception(std::forward<T>(text)), _error(error) {}
		Error getError() const noexcept { return _error; }
	private:
		Error const _error{ Error::NoError };
	};

	enum class QueryCommandError
	{
		EntityDescriptor,
		ConfigurationDescriptor,
		AudioUnitDescriptor,
		StreamInputDescriptor,
		StreamOutputDescriptor,
		AvbInterfaceDescriptor,
		ClockSourceDescriptor,
		MemoryObjectDescriptor,
		LocaleDescriptor,
		StringsDescriptor,
		StreamPortInputDescriptor,
		StreamPortOutputDescriptor,
		AudioClusterDescriptor,
		AudioMapDescriptor,
		ClockDomainDescriptor,
		StreamInputAudioMap,
		StreamOutputAudioMap,
		TalkerStreamState,
		ListenerStreamState,
		TalkerStreamConnection,
		TalkerStreamInfo,
		ListenerStreamInfo,
		AvbInfo,
		AsPath,
	};

	/**
	* @brief Observer for entity state and query results. All handlers are guaranteed to be mutually exclusively called.
	* @warning For all handlers, the la::avdecc::controller::ControlledEntity parameter should not be copied, since there
	*          is no guaranty it will still be valid upon return (although it's guaranteed to be valid for the duration of
	*          the handler). If you later need to get a new temporary pointer to it call the getControlledEntity method.
	*/
	class Observer : public la::avdecc::Observer<Controller>
	{
	public:
		// Global notifications
		virtual void onTransportError(la::avdecc::controller::Controller const* const /*controller*/) noexcept {}
		virtual void onEntityQueryError(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::Controller::QueryCommandError const /*error*/) noexcept {} // Might trigger even if entity is not "online" // Triggered when the controller failed to query all information it needs for an entity to be declared as Online
		// Discovery notifications (ADP)
		virtual void onEntityOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept {}
		virtual void onEntityOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept {}
		virtual void onGptpChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::UniqueIdentifier const /*grandMasterID*/, std::uint8_t const /*grandMasterDomain*/) noexcept {}
		// Connection notifications (ACMP)
		virtual void onStreamConnectionChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::model::StreamConnectionState const& /*state*/, bool const /*changedByOther*/) noexcept {}
		virtual void onStreamConnectionsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::controller::model::StreamConnections const& /*connections*/) noexcept {}
		// Entity model notifications (unsolicited AECP or changes this controller sent)
		virtual void onAcquireStateChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::AcquireState const /*acquireState*/, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept {} // TODO: We should later have a descriptor and index passed too, so we now which part of the EM graph has been acquired
		virtual void onStreamInputFormatChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		virtual void onStreamOutputFormatChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		virtual void onStreamInputInfoChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
		virtual void onStreamOutputInfoChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
		virtual void onEntityNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept {}
		virtual void onEntityGroupNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept {}
		virtual void onConfigurationNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept {}
		virtual void onStreamInputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
		virtual void onStreamOutputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
		virtual void onAudioUnitSamplingRateChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
		virtual void onClockSourceChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/) noexcept {}
		virtual void onStreamInputStarted(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		virtual void onStreamOutputStarted(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		virtual void onStreamInputStopped(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		virtual void onStreamOutputStopped(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		virtual void onAvbInfoChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInfo const& /*info*/) noexcept {}
		virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/) noexcept {}
		virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/) noexcept {}
	};

	/* Enumeration and Control Protocol (AECP) handlers. WARNING: The 'entity' parameter might be nullptr even if 'status' is AemCommandStatus::Success, in case the unit goes offline right after processing our command. */
	using AcquireEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity)>;
	using ReleaseEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity)>;
	using SetConfigurationHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetEntityNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamInputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamOutputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetClockSourceHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StartStreamInputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StopStreamInputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using AddStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using AddStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using RemoveStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using RemoveStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	using DisconnectTalkerStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status)>;

	/**
	* @brief Factory method to create a new Controller.
	* @details Creates a new Controller as a unique pointer.
	* @param[in] protocolInterfaceType The protocol interface type to use.
	* @param[in] interfaceName The name of the interface to bind the controller to.
	*            Use #la::avdecc::networkInterface::enumerateInterfaces to get a list of valid
	*            interfaces, and pass the 'name' field of a #la::avdecc::networkInterface::Interface
	*            to this method.
	* @param[in] progID ID that will be used to generate the #UniqueIdentifier for this controller.
	* @param[in] vendorEntityModelID VendorEntityModelID to publish for this controller. You can use entity::model::makeVendorEntityModel to create this value.
	* @param[in] preferedLocale ISO 639-1 locale code of the prefered locale to use when querying entity information.
	*                           If the specified locale is not found on the entity, then english is used.
	* @return A new Controller as a Controller::UniquePointer.
	* @note Throws Exception if interfaceName is invalid or inaccessible, or if progID is already used on the local computer.
	*/
	static UniquePointer create(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale)
	{
		auto deleter = [](Controller* controller)
		{
			controller->destroy();
		};
		return UniquePointer(createRawController(protocolInterfaceType, interfaceName, progID, vendorEntityModelID, preferedLocale), deleter);
	}

	/** Returns the UniqueIdentifier this instance of the controller is using to identify itself on the network */
	virtual UniqueIdentifier getControllerEID() const noexcept = 0;

	/* Controller configuration methods */
	/** Enables controller advertising with available duration included between 2-62 seconds. Might throw an Exception. */
	virtual void enableEntityAdvertising(std::uint32_t const availableDuration) = 0;

	/** Disables controller advertising. */
	virtual void disableEntityAdvertising() noexcept = 0;

	/* Enumeration and Control Protocol (AECP). WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP). WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept = 0;
	/** Sends a DisconnectTX message directly to the talker, spoofing the listener. Should only be used to forcefully disconnect a ghost connection on the talker. */
	virtual void disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept = 0;

	/** Gets a lock guarded ControlledEntity. While the returned object is in the scope, you are guaranteed to have exclusive access on the ControlledEntity. The returned guard should not be kept. */
	virtual ControlledEntityGuard getControlledEntity(UniqueIdentifier const entityID) const noexcept = 0;

	/** BasicLockable concept 'lock' method for the whole Controller */
	virtual void lock() noexcept = 0;
	/** BasicLockable concept 'unlock' method for the whole Controller */
	virtual void unlock() noexcept = 0;

	// Deleted compiler auto-generated methods
	Controller(Controller const&) = delete;
	Controller(Controller&&) = delete;
	Controller& operator=(Controller const&) = delete;
	Controller& operator=(Controller&&) = delete;

protected:
	/** Constructor */
	Controller() = default;

	/** Destructor */
	virtual ~Controller() = default;

private:
	/** Create method for COM-like interface */
	static LA_AVDECC_CONTROLLER_API Controller* LA_AVDECC_CONTROLLER_CALL_CONVENTION createRawController(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

/* Operator overloads */
constexpr bool operator!(Controller::Error const error)
{
	return error == Controller::Error::NoError;
}

} // namespace controller

// Define bitfield enum traits for controller::CompileOptions
template<>
struct enum_traits<controller::CompileOptions>
{
	static constexpr bool is_bitfield = true;
};

} // namespace avdecc
} // namespace la
