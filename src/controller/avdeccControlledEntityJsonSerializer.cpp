/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file avdeccControlledEntityJsonSerializer.cpp
* @author Christophe Calmejane
*/


#include "avdeccControllerJsonTypes.hpp"
#include "avdeccControlledEntityJsonSerializer.hpp"
#include "avdeccControllerImpl.hpp"
#include "avdeccControlledEntityImpl.hpp"

#include <la/avdecc/internals/jsonTypes.hpp>

using json = nlohmann::json;

namespace la
{
namespace avdecc
{
namespace controller
{
namespace jsonSerializer
{
/* ************************************************************ */
/* Public methods                                               */
/* ************************************************************ */
json createJsonObject(ControlledEntityImpl const& entity, bool const ignoreSanityChecks)
{
	try
	{
		// Create the object
		auto object = json{};
		auto const& e = entity.getEntity();

		// Dump information of the dump itself
		object[keyName::ControlledEntity_DumpVersion] = keyValue::ControlledEntity_DumpVersion;

		// Dump ADP information
		{
			auto& adp = object[keyName::ControlledEntity_AdpInformation];

			// Dump common information
			adp[entity::keyName::Entity_CommonInformation_Node] = e.getCommonInformation();

			// Dump interfaces information
			auto intfcs = json{};
			for (auto const& [avbInterfaceIndex, intfcInfo] : e.getInterfacesInformation()) // Don't use default std::map serializer, we want to force an array of object that includes the key (AvbInterfaceIndex)
			{
				json j = intfcInfo; // Must use operator= instead of constructor to force usage of the to_json overload
				if (avbInterfaceIndex == entity::Entity::GlobalAvbInterfaceIndex)
				{
					j[entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex] = nullptr;
				}
				else
				{
					j[entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex] = avbInterfaceIndex;
				}
				intfcs.push_back(j);
			}
			adp[entity::keyName::Entity_InterfaceInformation_Node] = intfcs;
		}

		// Dump device compatibility flags
		object[keyName::ControlledEntity_CompatibilityFlags] = entity.getCompatibilityFlags();

		// Dump AEM if supported
		if (e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
		{
			// Dump static and dynamic models
			auto flags = entity::model::jsonSerializer::Flags{ entity::model::jsonSerializer::Flag::ProcessStaticModel, entity::model::jsonSerializer::Flag::ProcessDynamicModel };
			if (ignoreSanityChecks)
			{
				flags.set(entity::model::jsonSerializer::Flag::IgnoreSanityChecks);
			}
			object[keyName::ControlledEntity_EntityModel] = entity::model::jsonSerializer::createJsonObject(entity.getEntityTree(), flags);
		}

		// Dump Milan information, if present
		auto const milanInfo = entity.getMilanInfo();
		if (milanInfo)
		{
			object[keyName::ControlledEntity_MilanInformation] = *milanInfo;
		}

		// Dump Entity State
		{
			auto& state = object[keyName::ControlledEntity_EntityState];
			state[controller::keyName::ControlledEntityState_AcquireState] = entity.getAcquireState();
			state[controller::keyName::ControlledEntityState_OwningControllerID] = entity.getOwningControllerID();
			state[controller::keyName::ControlledEntityState_LockState] = entity.getLockState();
			state[controller::keyName::ControlledEntityState_LockingControllerID] = entity.getLockingControllerID();
			state[controller::keyName::ControlledEntityState_SubscribedUnsol] = entity.isSubscribedToUnsolicitedNotifications();
			state[controller::keyName::ControlledEntityState_ActiveConfiguration] = entity.getCurrentConfigurationIndex();
		}

		// Dump Entity Statistics
		{
			auto& statistics = object[keyName::ControlledEntity_Statistics];
			statistics[controller::keyName::ControlledEntityStatistics_AecpRetryCounter] = entity.getAecpRetryCounter();
			statistics[controller::keyName::ControlledEntityStatistics_AecpTimeoutCounter] = entity.getAecpTimeoutCounter();
			statistics[controller::keyName::ControlledEntityStatistics_AecpUnexpectedResponseCounter] = entity.getAecpUnexpectedResponseCounter();
			statistics[controller::keyName::ControlledEntityStatistics_AecpResponseAverageTime] = entity.getAecpResponseAverageTime();
			statistics[controller::keyName::ControlledEntityStatistics_AemAecpUnsolicitedCounter] = entity.getAemAecpUnsolicitedCounter();
			statistics[controller::keyName::ControlledEntityStatistics_EnumerationTime] = entity.getEnumerationTime();
		}

		return object;
	}
	catch (json::exception const& e)
	{
		AVDECC_ASSERT(false, "json::exception is not expected to be thrown here");
		throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InternalError, e.what() };
	}
	catch (avdecc::jsonSerializer::SerializationException const&)
	{
		throw; // Rethrow, this is already the correct exception type
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::SerializationException are not expected to be thrown here");
		throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InternalError, "Exception type other than avdecc::jsonSerializer::SerializationException are not expected to be thrown here." };
	}
}

void setEntityModel(ControlledEntityImpl& entity, json const& object, entity::model::jsonSerializer::Flags flags)
{
	try
	{
		// Read AEM if supported
		if (entity.getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported))
		{
			// Read Entity Tree
			auto entityTree = entity::model::jsonSerializer::createEntityTree(object, flags);

			// Set tree on entity
			entity.setEntityTree(entityTree);
		}
	}
	catch (json::exception const& e)
	{
		AVDECC_ASSERT(false, "json::exception is not expected to be thrown here");
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, e.what() };
	}
	catch (avdecc::jsonSerializer::DeserializationException const&)
	{
		throw; // Rethrow, this is already the correct exception type
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here");
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here." };
	}
}

void setEntityState(ControlledEntityImpl& entity, json const& object)
{
	try
	{
		// Everything is optional, except for the current configuration
		{
			auto const it = object.find(controller::keyName::ControlledEntityState_AcquireState);
			if (it != object.end())
			{
				entity.setAcquireState(it->get<model::AcquireState>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityState_OwningControllerID);
			if (it != object.end())
			{
				entity.setOwningController(it->get<UniqueIdentifier>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityState_LockState);
			if (it != object.end())
			{
				entity.setLockState(it->get<model::LockState>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityState_LockingControllerID);
			if (it != object.end())
			{
				entity.setLockingController(it->get<UniqueIdentifier>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityState_SubscribedUnsol);
			if (it != object.end())
			{
				entity.setSubscribedToUnsolicitedNotifications(it->get<bool>());
			}
		}
		entity.setCurrentConfiguration(object.at(controller::keyName::ControlledEntityState_ActiveConfiguration).get<entity::model::DescriptorIndex>());
	}
	catch (json::type_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what() };
	}
	catch (json::parse_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::ParseError, e.what() };
	}
	catch (json::out_of_range const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::MissingKey, e.what() };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what() };
		}
		else
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
		}
	}
	catch (json::exception const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
	}
	catch (avdecc::jsonSerializer::DeserializationException const&)
	{
		throw; // Rethrow, this is already the correct exception type
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here");
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here." };
	}
}

void setEntityStatistics(ControlledEntityImpl& entity, json const& object)
{
	try
	{
		// Everything is optional
		{
			auto const it = object.find(controller::keyName::ControlledEntityStatistics_AecpRetryCounter);
			if (it != object.end())
			{
				entity.setAecpRetryCounter(it->get<std::uint64_t>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityStatistics_AecpTimeoutCounter);
			if (it != object.end())
			{
				entity.setAecpTimeoutCounter(it->get<std::uint64_t>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityStatistics_AecpUnexpectedResponseCounter);
			if (it != object.end())
			{
				entity.setAecpUnexpectedResponseCounter(it->get<std::uint64_t>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityStatistics_AecpResponseAverageTime);
			if (it != object.end())
			{
				entity.setAecpResponseAverageTime(it->get<std::chrono::milliseconds>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityStatistics_AemAecpUnsolicitedCounter);
			if (it != object.end())
			{
				entity.setAemAecpUnsolicitedCounter(it->get<std::uint64_t>());
			}
		}
		{
			auto const it = object.find(controller::keyName::ControlledEntityStatistics_EnumerationTime);
			if (it != object.end())
			{
				entity.setEnumerationTime(it->get<std::chrono::milliseconds>());
			}
		}
	}
	catch (json::type_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what() };
	}
	catch (json::parse_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::ParseError, e.what() };
	}
	catch (json::out_of_range const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::MissingKey, e.what() };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what() };
		}
		else
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
		}
	}
	catch (json::exception const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
	}
	catch (avdecc::jsonSerializer::DeserializationException const&)
	{
		throw; // Rethrow, this is already the correct exception type
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here");
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here." };
	}
}

} // namespace jsonSerializer
} // namespace controller
} // namespace avdecc
} // namespace la
