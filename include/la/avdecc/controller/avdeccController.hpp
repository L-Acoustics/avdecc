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
* @file avdeccController.hpp
* @author Christophe Calmejane
* @brief Avdecc controller.
*/

#pragma once

#include <memory>
#include <functional>
#include <string>
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
constexpr std::uint32_t InterfaceVersion = 200;

/**
* @brief Checks if the library is compatible with specified interface version.
* @param[in] interfaceVersion The interface version to check for compatibility.
* @return True if the library is compatible.
* @note If the library is not compatible, the application should no longer use the library.<BR>
*       When using the avdecc shared library, you must call this version to check the compatibility between the compiled and the loaded version.
*/
LA_AVDECC_CONTROLLER_API bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept;

/**
* @brief Gets the avdecc library version.
* @details Returns the avdecc library version.
* @return A string representing the library version.
*/
LA_AVDECC_CONTROLLER_API std::string LA_AVDECC_CONTROLLER_CALL_CONVENTION getVersion() noexcept;

/**
* @brief Gets the avdecc shared library interface version.
* @details Returns the avdecc shared library interface version.
* @return The interface version.
*/
LA_AVDECC_CONTROLLER_API std::uint32_t LA_AVDECC_CONTROLLER_CALL_CONVENTION getInterfaceVersion() noexcept;

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
		LocaleDescriptor,
		StringsDescriptor,
		StreamInputDescriptor,
		StreamOutputDescriptor,
		StreamInputAudioMap,
		StreamOutputAudioMap,
		ListenerStreamState,
	};

	/**
	* @brief Observer for entity state and query results. All handlers are guaranteed to be mutually exclusively called.
	* @warning For all handlers, the la::avdecc::controller::ControlledEntity parameter should not be copied, since there
	*          is no guaranty it will still be valid upon return. If you later need to get a new temporary pointer to it
	*          call the runMethodForEntity.
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
		// Connection notifications (sniffed ACMP)
		virtual void onConnectStreamSniffed(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*talkerEntity*/, la::avdecc::controller::ControlledEntity const* const /*listenerEntity*/, la::avdecc::entity::model::StreamIndex const /*talkerStreamIndex*/, la::avdecc::entity::model::StreamIndex const /*listenerStreamIndex*/) noexcept {}
		virtual void onDisconnectStreamSniffed(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*listenerEntity*/, la::avdecc::entity::model::StreamIndex const /*listenerStreamIndex*/) noexcept {}
		// Entity model notifications (unsolicited AECP or changes this controller sent)
		virtual void onAcquireStateChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::ControlledEntity::AcquireState const /*acquireState*/, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept {} // TBD: We should later have a descriptor and index passed too, so we now which part of the EM graph has been acquired
		virtual void onStreamInputFormatChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		virtual void onStreamOutputFormatChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		virtual void onStreamInputAudioMappingsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		virtual void onStreamOutputAudioMappingsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		virtual void onEntityNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept {}
		virtual void onEntityGroupNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept {}
		virtual void onConfigurationNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept {}
		virtual void onStreamInputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
		virtual void onStreamOutputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
	};

	/* Enumeration and Control Protocol (AECP) handlers. WARNING: The 'entity' parameter might be nullptr even if 'status' is AemCommandStatus::Success, in case the unit goes offline right after processing our command. */
	using AcquireEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity) noexcept>;
	using ReleaseEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity) noexcept>;
	using SetConfigurationHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using AddStreamInputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using AddStreamOutputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using RemoveStreamInputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using RemoveStreamOutputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using StartStreamInputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using StopStreamInputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetEntityNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetStreamInputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using SetStreamOutputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using RunMethodForEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity) noexcept>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;

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

	/* Controller configuration methods */
	/** Enables controller advertising with available duration included between 2-62 seconds. Might throw an Exception. */
	virtual void enableEntityAdvertising(std::uint32_t const availableDuration) = 0;

	/** Disables controller advertising. */
	virtual void disableEntityAdvertising() noexcept = 0;

	/* Enumeration and Control Protocol (AECP). WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void acquireEntity(la::avdecc::UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(la::avdecc::UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void setConfiguration(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void addStreamInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, std::vector<la::avdecc::entity::model::AudioMapping> const& mappings, AddStreamInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, std::vector<la::avdecc::entity::model::AudioMapping> const& mappings, RemoveStreamInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;
	virtual void setEntityName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void setStreamInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP). WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void connectStream(la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept = 0;

	/** Method that will call the handler with a temporary copy of the la::avdecc::controller::ControlledEntity matching entityID (which is nullptr if it does not exist). */
	virtual void runMethodForEntity(la::avdecc::UniqueIdentifier const entityID, RunMethodForEntityHandler const& handler) const = 0;

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
} // namespace avdecc
} // namespace la
