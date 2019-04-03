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

#include "la/avdecc/internals/jsonSerialization.hpp"
#include "la/avdecc/internals/jsonTypes.hpp"
#include "la/avdecc/controller/internals/jsonTypes.hpp"

#include "avdeccControlledEntityJsonSerializer.hpp"
#include "avdeccControllerImpl.hpp"
#include "avdeccControlledEntityImpl.hpp"

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
json createJsonObject(ControlledEntityImpl const& entity) noexcept
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
		for (auto const& [avbInterfaceIndex, intfcInfo] : e.getInterfacesInformation())
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
			object[entity::keyName::Entity_InterfaceInformation_Node].push_back(j);
		}
	}

	// Dump device compatibility flags
	object[keyName::ControlledEntity_CompatibilityFlags] = entity.getCompatibilityFlags();

	// Dump AEM if supported
	if (e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
	{
		// Dump static and dynamic models
		object[keyName::ControlledEntity_EntityModel] = la::avdecc::entity::model::jsonSerializer::createJsonObject(entity.getEntityTree(), la::avdecc::entity::model::jsonSerializer::SerializationFlags{ la::avdecc::entity::model::jsonSerializer::SerializationFlag::SerializeStaticModel, la::avdecc::entity::model::jsonSerializer::SerializationFlag::SerializeDynamicModel });
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
	}

	return object;
}

} // namespace jsonSerializer
} // namespace controller
} // namespace avdecc
} // namespace la
