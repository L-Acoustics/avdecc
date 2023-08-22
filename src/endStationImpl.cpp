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
* @file endStationImpl.cpp
* @author Christophe Calmejane
*/

#include "endStationImpl.hpp"
#include "entity/controllerEntityImpl.hpp"
#include "entity/aggregateEntityImpl.hpp"
#include "entity/aemHandler.hpp"

#include "la/avdecc/internals/protocolInterface.hpp"
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include "la/avdecc/internals/jsonTypes.hpp"
#endif // ENABLE_AVDECC_FEATURE_JSON

#include <fstream>

namespace la
{
namespace avdecc
{
EndStationImpl::EndStationImpl(ExecutorManager::ExecutorWrapper::UniquePointer&& executorWrapper, protocol::ProtocolInterface::UniquePointer&& protocolInterface) noexcept
	: _executorWrapper{ std::move(executorWrapper) }
	, _protocolInterface{ std::move(protocolInterface) }
{
}

EndStationImpl::~EndStationImpl() noexcept
{
	// Remove all entities before shuting down the protocol interface (so they have a chance to send a ENTITY_DEPARTING message)
	_entities.clear();
	// Shutdown protocolInterface now
	_protocolInterface->shutdown();
	// Destroy the executor right now (flushing all events), before the PI is destroyed (and possibly accessed from the executor)
	_executorWrapper = nullptr;
}

// EndStation overrides
entity::ControllerEntity* EndStationImpl::addControllerEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::model::EntityTree const* const entityModelTree, entity::controller::Delegate* const delegate)
{
	std::unique_ptr<entity::LocalEntityGuard<entity::ControllerEntityImpl>> controller{ nullptr };
	try
	{
		auto const eid = entity::Entity::generateEID(_protocolInterface->getMacAddress(), progID, false);

		// Validate EntityModel
		try
		{
			entity::model::AemHandler::validateEntityModel(entityModelTree);
		}
		catch (la::avdecc::Exception const& e)
		{
			throw Exception(Error::InvalidEntityModel, e.what());
		}

		try
		{
			auto entityCapabilities = entity::EntityCapabilities{};
			if (entityModelTree != nullptr)
			{
				entityCapabilities.set(entity::EntityCapability::AemSupported);
			}

			auto const commonInformation{ entity::Entity::CommonInformation{ eid, entityModelID, entityCapabilities, 0u, entity::TalkerCapabilities{}, 0u, entity::ListenerCapabilities{}, entity::ControllerCapabilities{ entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
			auto const interfaceInfo{ entity::Entity::InterfaceInformation{ _protocolInterface->getMacAddress(), 31u, 0u, std::nullopt, std::nullopt } };

			controller = std::make_unique<entity::LocalEntityGuard<entity::ControllerEntityImpl>>(_protocolInterface.get(), commonInformation, entity::Entity::InterfacesInformation{ { entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, entityModelTree, delegate);
		}
		catch (la::avdecc::Exception const& e) // Because entity::ControllerEntityImpl::ControllerEntityImpl might throw if an entityID cannot be generated
		{
			throw Exception(Error::DuplicateEntityID, e.what());
		}
	}
	catch (Exception const&)
	{
		throw; // Forward exceptions thrown by the inner bloc
	}
	catch (la::avdecc::Exception const& e) // entity::Entity::generateEID might throw if ProtocolInterface is not valid (doesn't have a valid MacAddress)
	{
		throw Exception(Error::InterfaceInvalid, e.what());
	}

	// Get controller's pointer now, we'll soon move the object
	auto* const controllerPtr = static_cast<entity::ControllerEntity*>(controller.get());

	// Add the entity to our list
	_entities.push_back(std::move(controller));

	// Return the controller to the user
	return controllerPtr;
}

entity::AggregateEntity* EndStationImpl::addAggregateEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::model::EntityTree const* const entityModelTree, entity::controller::Delegate* const controllerDelegate)
{
	std::unique_ptr<entity::LocalEntityGuard<entity::AggregateEntityImpl>> aggregate{ nullptr };
	try
	{
		auto const eid = entity::Entity::generateEID(_protocolInterface->getMacAddress(), progID, false);

		// Validate EntityModel
		try
		{
			entity::model::AemHandler::validateEntityModel(entityModelTree);
		}
		catch (la::avdecc::Exception const& e)
		{
			throw Exception(Error::InvalidEntityModel, e.what());
		}

		try
		{
			auto entityCapabilities = entity::EntityCapabilities{};
			if (entityModelTree != nullptr)
			{
				entityCapabilities.set(entity::EntityCapability::AemSupported);
			}

			auto const commonInformation{ entity::Entity::CommonInformation{ eid, entityModelID, entityCapabilities, 0u, entity::TalkerCapabilities{}, 0u, entity::ListenerCapabilities{}, controllerDelegate ? entity::ControllerCapabilities{ entity::ControllerCapability::Implemented } : entity::ControllerCapabilities{}, std::nullopt, std::nullopt } };
			auto const interfaceInfo{ entity::Entity::InterfaceInformation{ _protocolInterface->getMacAddress(), 31u, 0u, std::nullopt, std::nullopt } };

			aggregate = std::make_unique<entity::LocalEntityGuard<entity::AggregateEntityImpl>>(_protocolInterface.get(), commonInformation, entity::Entity::InterfacesInformation{ { entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, entityModelTree, controllerDelegate);
		}
		catch (la::avdecc::Exception const& e) // Because entity::AggregateEntityImpl::AggregateEntityImpl might throw if entityID is already locally registered
		{
			throw Exception(Error::DuplicateEntityID, e.what());
		}
	}
	catch (Exception const&)
	{
		throw; // Forward exceptions thrown by the inner bloc
	}
	catch (la::avdecc::Exception const& e) // entity::Entity::generateEID might throw if ProtocolInterface is not valid (doesn't have a valid MacAddress)
	{
		throw Exception(Error::InterfaceInvalid, e.what());
	}

	// Get aggregate's pointer now, we'll soon move the object
	auto* const aggregatePtr = static_cast<entity::AggregateEntity*>(aggregate.get());

	// Add the entity to our list
	_entities.push_back(std::move(aggregate));

	// Return the aggregate to the user
	return aggregatePtr;
}

protocol::ProtocolInterface const* EndStationImpl::getProtocolInterface() const noexcept
{
	return _protocolInterface.get();
}

/** Destroy method for COM-like interface */
void EndStationImpl::destroy() noexcept
{
	delete this;
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, entity::model::EntityTree> LA_AVDECC_CALL_CONVENTION EndStation::deserializeEntityModelFromJson([[maybe_unused]] std::string const& filePath, [[maybe_unused]] bool const processDynamicModel, [[maybe_unused]] bool const isBinaryFormat) noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::DeserializationError::NotSupported, "Deserialization feature not supported by the library (was not compiled)", entity::model::EntityTree{} };

#else // ENABLE_AVDECC_FEATURE_JSON

	auto flags = entity::model::jsonSerializer::Flags{ entity::model::jsonSerializer::Flag::ProcessStaticModel };
	if (processDynamicModel)
	{
		flags.set(entity::model::jsonSerializer::Flag::ProcessDynamicModel);
	}
	if (isBinaryFormat)
	{
		flags.set(entity::model::jsonSerializer::Flag::BinaryFormat);
	}

	// Try to open the input file
	auto const mode = std::ios::binary | std::ios::in;
	auto ifs = std::ifstream{ utils::filePathFromUTF8String(filePath), mode }; // We always want to read as 'binary', we don't want the cr/lf shit to alter the size of our allocated buffer (all modern code should handle both lf and cr/lf)

	// Failed to open file for reading
	if (!ifs.is_open())
	{
		return { avdecc::jsonSerializer::DeserializationError::AccessDenied, std::strerror(errno), entity::model::EntityTree{} };
	}

	// Load the JSON object from disk
	auto object = json{};
	try
	{
		if (flags.test(entity::model::jsonSerializer::Flag::BinaryFormat))
		{
			object = json::from_msgpack(ifs);
		}
		else
		{
			ifs >> object;
		}
	}
	catch (json::type_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what(), entity::model::EntityTree{} };
	}
	catch (json::parse_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::ParseError, e.what(), entity::model::EntityTree{} };
	}
	catch (json::out_of_range const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::MissingKey, e.what(), entity::model::EntityTree{} };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			return { avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what(), entity::model::EntityTree{} };
		}
		else
		{
			return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), entity::model::EntityTree{} };
		}
	}
	catch (json::exception const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), entity::model::EntityTree{} };
	}

	// Try to deserialize
	try
	{
		auto entityTree = entity::model::jsonSerializer::createEntityTree(object, flags);
		return { avdecc::jsonSerializer::DeserializationError::NoError, "", entityTree };
	}
	catch (json::exception const& e)
	{
		AVDECC_ASSERT(false, "json::exception is not expected to be thrown here");
		return { avdecc::jsonSerializer::DeserializationError::InternalError, e.what(), entity::model::EntityTree{} };
	}
	catch (avdecc::jsonSerializer::DeserializationException const& e)
	{
		return { e.getError(), e.what(), entity::model::EntityTree{} };
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here");
		return { avdecc::jsonSerializer::DeserializationError::InternalError, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here.", entity::model::EntityTree{} };
	}
#endif // ENABLE_AVDECC_FEATURE_JSON
}

/** EndStation Entry point */
EndStation* LA_AVDECC_CALL_CONVENTION EndStation::createRawEndStation(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceName, std::optional<std::string> const& executorName)
{
	try
	{
		static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

		auto executorWrapper = ExecutorManager::ExecutorWrapper::UniquePointer{ nullptr, nullptr };
		auto exName = std::string{ DefaultExecutorName };

		// If we are passed an executor name, check if it exists
		if (executorName.has_value())
		{
			// Executor name was passed, check if it exists
			if (!ExecutorManager::getInstance().isExecutorRegistered(*executorName))
			{
				// Executor does not exist
				throw Exception(Error::UnknownExecutorName, "Executor not found");
			}
			// Executor exists, we can use it
			exName = *executorName;
		}
		// Create the executor and manage it's lifetime
		else
		{
			// First check if the executor already exists
			if (ExecutorManager::getInstance().isExecutorRegistered(exName))
			{
				// Executor already exists, we can't create a new one
				throw Exception(Error::DuplicateExecutorName, "Executor already exists");
			}
			// We can create a new executor
			executorWrapper = ExecutorManager::getInstance().registerExecutor(exName, ExecutorWithDispatchQueue::create(exName, utils::ThreadPriority::Highest));
		}

		return new EndStationImpl(std::move(executorWrapper), protocol::ProtocolInterface::create(protocolInterfaceType, networkInterfaceName, exName));
	}
	catch (protocol::ProtocolInterface::Exception const& e)
	{
		auto const err = e.getError();
		switch (err)
		{
			case protocol::ProtocolInterface::Error::TransportError:
				throw Exception(Error::InterfaceOpenError, e.what());
			case protocol::ProtocolInterface::Error::InterfaceNotFound:
				throw Exception(Error::InterfaceNotFound, e.what());
			case protocol::ProtocolInterface::Error::InvalidParameters:
				throw Exception(Error::InterfaceInvalid, e.what());
			case protocol::ProtocolInterface::Error::InterfaceNotSupported:
				throw Exception(Error::InvalidProtocolInterfaceType, e.what());
			case protocol::ProtocolInterface::Error::ExecutorNotInitialized:
				throw Exception(Error::InternalError, e.what()); // Should never happen, the EndStation registers the executor in the constructor
			case protocol::ProtocolInterface::Error::InternalError:
				throw Exception(Error::InternalError, e.what());
			default:
				AVDECC_ASSERT(false, "Unhandled exception");
				throw Exception(Error::InternalError, e.what());
		}
	}
	catch (std::runtime_error const& e)
	{
		throw Exception(Error::InterfaceOpenError, e.what());
	}
}

} // namespace avdecc
} // namespace la
