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
* @file endStation.hpp
* @author Christophe Calmejane
* @brief Avdecc EndStation.
*/

#pragma once

#include "entity.hpp"
#include "controllerEntity.hpp"
#include "aggregateEntity.hpp"
#include "protocolInterface.hpp"
#include "exports.hpp"
#include "exception.hpp"
#include <cstdint>
#include <memory>
#include <string>

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
	* @param[in] networkInterfaceName The name of the network interface to use. Use #la::avdecc::networkInterface::enumerateInterfaces to get a valid interface name.
	* @return A new EndStation as a EndStation::UniquePointer.
	* @note Might throw an Exception.
	* @warning This class is currently NOT thread-safe.
	*/
	static UniquePointer create(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceName)
	{
		auto deleter = [](EndStation* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawEndStation(protocolInterfaceType, networkInterfaceName), deleter);
	}

	/**
	* @brief Create and attach a controller entity to the EndStation.
	* @details Creates and attaches a controller type entity to the EndStation.
	* @param[in] progID ID that will be used to generate the #UniqueIdentifier for the controller.
	* @param[in] entityModelID The EntityModelID value for the controller. You can use entity::model::makeEntityModelID to create this value.
	* @param[in] delegate The Delegate to be called whenever a controller related notification occurs.
	* @return A weak pointer to the newly created ControllerEntity.
	* @note Might throw an Exception.
	*/
	virtual entity::ControllerEntity* addControllerEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::controller::Delegate* const delegate) = 0;

	virtual entity::AggregateEntity* addAggregateEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::controller::Delegate* const controllerDelegate) = 0;

	// Deleted compiler auto-generated methods
	EndStation(EndStation&&) = delete;
	EndStation(EndStation const&) = delete;
	EndStation& operator=(EndStation const&) = delete;
	EndStation& operator=(EndStation&&) = delete;

protected:
	/** Constructor */
	EndStation() noexcept = default;

	/** Destructor */
	virtual ~EndStation() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API EndStation* LA_AVDECC_CALL_CONVENTION createRawEndStation(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceName);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

} // namespace avdecc
} // namespace la
