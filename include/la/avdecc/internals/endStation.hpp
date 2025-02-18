/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file endStation.hpp
* @author Christophe Calmejane
* @brief Avdecc EndStation.
*/

#pragma once

#include "entity.hpp"
#include "controllerEntity.hpp"
#include "aggregateEntity.hpp"
#include "protocolInterface.hpp"
#include "entityModelTree.hpp"
#include "exports.hpp"
#include "exception.hpp"
#include "jsonSerialization.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <optional>

namespace la
{
namespace avdecc
{
class EndStation
{
public:
	enum class Error
	{
		NoError = 0,
		InvalidProtocolInterfaceType = 1, /**< Selected protocol interface type is invalid. */
		InterfaceOpenError = 2, /**< Failed to open interface. */
		InterfaceNotFound = 3, /**< Specified interface not found. */
		InterfaceInvalid = 4, /**< Specified interface is invalid. */
		DuplicateEntityID = 5, /**< EntityID not available (either duplicate, or no EntityID left on the local computer). */
		InvalidEntityModel = 6, /**< Provided EntityModel is invalid. */
		DuplicateExecutorName = 7, /**< Provided executor name already exists. */
		UnknownExecutorName = 8, /**< Provided executor name doesn't exist. */
		InternalError = 99, /**< Internal error, please report the issue. */
	};

	class LA_AVDECC_API Exception final : public la::avdecc::Exception
	{
	public:
		template<class T>
		Exception(Error const error, T&& text) noexcept
			: la::avdecc::Exception(std::forward<T>(text))
			, _error(error)
		{
		}
		Error getError() const noexcept
		{
			return _error;
		}

	private:
		Error const _error{ Error::NoError };
	};

	using UniquePointer = std::unique_ptr<EndStation, void (*)(EndStation*)>;

	/**
	* @brief Factory method to create a new EndStation.
	* @details Creates a new EndStation as a unique pointer.
	* @param[in] protocolInterfaceType The protocol interface type to use.
	* @param[in] networkInterfaceID The ID of the network interface to use. Use #la::avdecc::networkInterface::enumerateInterfaces to get a valid interface ID.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages (must be created before the call). If empty, a default executor will be created.
	* @return A new EndStation as a EndStation::UniquePointer.
	* @note Might throw an Exception.
	* @warning This class is currently NOT thread-safe.
	*/
	static UniquePointer create(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceID, std::optional<std::string> const& executorName)
	{
		auto deleter = [](EndStation* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawEndStation(protocolInterfaceType, networkInterfaceID, executorName), deleter);
	}

	/**
	* @brief Create and attach a controller entity to the EndStation.
	* @details Creates and attaches a controller type entity to the EndStation.
	* @param[in] progID ID that will be used to generate the #UniqueIdentifier for the controller.
	* @param[in] entityModelID The EntityModelID value for the controller. You can use entity::model::makeEntityModelID to create this value.
	* @param[in] entityModelTree The entity model tree to use for this controller entity, or null to not expose a model.
	* @param[in] delegate The Delegate to be called whenever a controller related notification occurs.
	* @return A weak pointer to the newly created ControllerEntity.
	* @note Might throw an Exception.
	*/
	virtual entity::ControllerEntity* addControllerEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::model::EntityTree const* const entityModelTree, entity::controller::Delegate* const delegate) = 0;

	// TODO: Add all other AggregateEntity parameters
	virtual entity::AggregateEntity* addAggregateEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::model::EntityTree const* const entityModelTree, entity::controller::Delegate* const controllerDelegate) = 0;

	/** Returns the protocol interface used by this EndStation. */
	virtual protocol::ProtocolInterface const* getProtocolInterface() const noexcept = 0;

	/** Deserializes a JSON file representing an entity model, and returns the model without loading it. */
	static LA_AVDECC_API std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, entity::model::EntityTree> LA_AVDECC_CALL_CONVENTION deserializeEntityModelFromJson(std::string const& filePath, bool const processDynamicModel, bool const isBinaryFormat) noexcept;

	// Deleted compiler auto-generated methods
	EndStation(EndStation&&) = delete;
	EndStation(EndStation const&) = delete;
	EndStation& operator=(EndStation const&) = delete;
	EndStation& operator=(EndStation&&) = delete;

	/** Destructor */
	virtual ~EndStation() noexcept = default;

protected:
	/** Constructor */
	EndStation() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API EndStation* LA_AVDECC_CALL_CONVENTION createRawEndStation(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceID, std::optional<std::string> const& executorName);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

} // namespace avdecc
} // namespace la
