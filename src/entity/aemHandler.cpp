/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file aemHandler.cpp
* @author Christophe Calmejane
*/

#include "aemHandler.hpp"
#include "entityImpl.hpp"
#include "protocol/protocolAemPayloads.hpp"


namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
AemHandler::AemHandler(entity::Entity const& entity, entity::model::EntityTree const* const entityModelTree) noexcept
	: _entity{ entity }
	, _entityModelTree{ entityModelTree }
{
}

bool AemHandler::onUnhandledAecpAemCommand(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aem) const noexcept
{
	static std::unordered_map<protocol::AemCommandType::value_type, std::function<bool(protocol::ProtocolInterface* const pi, AemHandler const& aemHandler, protocol::AemAecpdu const& aem)>> s_Dispatch{
		// Read Descriptor
		{ protocol::AemCommandType::ReadDescriptor.getValue(),
			[](protocol::ProtocolInterface* const pi, AemHandler const& aemHandler, protocol::AemAecpdu const& aem)
			{
				if (aemHandler._entityModelTree != nullptr)
				{
					auto const [configIndex, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeReadDescriptorCommand(aem.getPayload());
					switch (descriptorType)
					{
						case DescriptorType::Entity:
						{
							if (configIndex != DescriptorIndex{ 0u } || descriptorIndex != DescriptorIndex{ 0u })
							{
								LocalEntityImpl<>::reflectAecpCommand(pi, aem, protocol::AemAecpStatus::BadArguments);
								return true;
							}
							auto ser = protocol::aemPayload::serializeReadDescriptorCommonResponse(configIndex, descriptorType, descriptorIndex);
							protocol::aemPayload::serializeReadEntityDescriptorResponse(ser, aemHandler.buildEntityDescriptor());
							LocalEntityImpl<>::sendAemAecpResponse(pi, aem, protocol::AemAecpStatus::Success, ser.data(), ser.size());
							return true;
						}
						default:
							break;
					}
				}
				return false;
			} },
	};

	auto const& it = s_Dispatch.find(aem.getCommandType().getValue());
	if (it != s_Dispatch.end())
	{
		try
		{
			return invokeProtectedHandler(it->second, pi, *this, aem);
		}
		catch (la::avdecc::Exception const&)
		{
			LocalEntityImpl<>::reflectAecpCommand(pi, aem, protocol::AemAecpStatus::EntityMisbehaving);
			return true;
		}
	}

	return false;
}

EntityDescriptor AemHandler::buildEntityDescriptor() const noexcept
{
	auto entityDescriptor = EntityDescriptor{};


	entityDescriptor.entityID = _entity.getEntityID();
	entityDescriptor.entityModelID = _entity.getEntityModelID();
	entityDescriptor.entityCapabilities = _entity.getEntityCapabilities();
	entityDescriptor.talkerStreamSources = _entity.getTalkerStreamSources();
	entityDescriptor.talkerCapabilities = _entity.getTalkerCapabilities();
	entityDescriptor.listenerStreamSinks = _entity.getListenerStreamSinks();
	entityDescriptor.listenerCapabilities = _entity.getListenerCapabilities();
	entityDescriptor.controllerCapabilities = _entity.getControllerCapabilities();
	entityDescriptor.availableIndex = 0u;
	if (auto const id = _entity.getAssociationID(); id.has_value())
	{
		entityDescriptor.associationID = *id;
	}
	entityDescriptor.entityName = _entityModelTree->dynamicModel.entityName;
	entityDescriptor.vendorNameString = _entityModelTree->staticModel.vendorNameString;
	entityDescriptor.modelNameString = _entityModelTree->staticModel.modelNameString;
	entityDescriptor.firmwareVersion = _entityModelTree->dynamicModel.firmwareVersion;
	entityDescriptor.groupName = _entityModelTree->dynamicModel.groupName;
	entityDescriptor.serialNumber = _entityModelTree->dynamicModel.serialNumber;
	entityDescriptor.configurationsCount = static_cast<decltype(entityDescriptor.configurationsCount)>(_entityModelTree->configurationTrees.size());
	entityDescriptor.currentConfiguration = _entityModelTree->dynamicModel.currentConfiguration;

	return entityDescriptor;
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la