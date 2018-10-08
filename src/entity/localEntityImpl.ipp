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
* @file localEntityImpl.ipp
* @author Christophe Calmejane
* @brief Implementation file of LocalEntityImpl template class.
*/

#pragma once

namespace la
{
namespace avdecc
{
namespace entity
{
template<class SuperClass>
LocalEntity::AemCommandStatus LocalEntityImpl<SuperClass>::convertErrorToAemCommandStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return AemCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return AemCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return AemCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAemAecpCommand from a non-existing local entity");
			return AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendAemAecpCommand from a non-controller entity");
			return AemCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return AemCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return AemCommandStatus::InternalError;
}

template<class SuperClass>
LocalEntity::AaCommandStatus LocalEntityImpl<SuperClass>::convertErrorToAaCommandStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return AaCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return AaCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return AaCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return AaCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAaCommand from a non-existing local entity");
			return AaCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendAaCommand from a non-controller entity");
			return AaCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return AaCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return AaCommandStatus::InternalError;
}

template<class SuperClass>
LocalEntity::MvuCommandStatus LocalEntityImpl<SuperClass>::convertErrorToMvuCommandStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return MvuCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return MvuCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return MvuCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return MvuCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAaCommand from a non-existing local entity");
			return MvuCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendAaCommand from a non-controller entity");
			return MvuCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return MvuCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return MvuCommandStatus::InternalError;
}

template<class SuperClass>
LocalEntity::ControlStatus LocalEntityImpl<SuperClass>::convertErrorToControlStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return ControlStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return ControlStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return ControlStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAcmpCommand from a non-existing local entity");
			return ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			return ControlStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return ControlStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return ControlStatus::InternalError;
}

template<class SuperClass>
void LocalEntityImpl<SuperClass>::onAecpCommand(protocol::ProtocolInterface* const pi, LocalEntity const& /*entity*/, protocol::Aecpdu const& aecpdu) noexcept
{
	// Ignore messages not for me
	if (aecpdu.getTargetEntityID() != SuperClass::getEntityID())
		return;

	if (aecpdu.getMessageType() == protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);

#pragma message("TODO: Code to be removed once checked it never happens (This is a LocalEntity, not a ControllerEntity, we should not check anything Controller specific here)")
		auto const controllerID = aem.getControllerEntityID();
		// Filter self messages
		if (!AVDECC_ASSERT_WITH_RET(controllerID != SuperClass::getEntityID(), "Receiving a message from ourself?"))
			return;

		static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(protocol::ProtocolInterface* const pi, LocalEntityImpl const* const entity, protocol::AemAecpdu const& aem)>> s_Dispatch{
			// Entity Available
			{ protocol::AemCommandType::EntityAvailable.getValue(),
				[](protocol::ProtocolInterface* const pi, auto const* const entity, protocol::AemAecpdu const& aem)
				{
					// We are being asked if we are available, and we are! Reply that
					entity->sendAemAecpResponse(pi, aem, protocol::AemAecpStatus::Success, nullptr, 0u);
				} },
		};

		auto const& it = s_Dispatch.find(aem.getCommandType().getValue());
		if (it != s_Dispatch.end())
		{
			invokeProtectedHandler(it->second, pi, this, aem);
			return;
		}
	}

	if (!onUnhandledAecpCommand(pi, aecpdu))
	{
		// Reflect back the command, and return a NotSupported error code
		reflectAecpCommand(pi, aecpdu, protocol::AemAecpStatus::NotSupported);
	}
}

} // namespace entity
} // namespace avdecc
} // namespace la
