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
			return LocalEntity::AemCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return LocalEntity::AemCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return LocalEntity::AemCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return LocalEntity::AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAemAecpCommand from a non-existing local entity");
			return LocalEntity::AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendAemAecpCommand from a non-controller entity");
			return LocalEntity::AemCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return LocalEntity::AemCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return LocalEntity::AemCommandStatus::InternalError;
}

template<class SuperClass>
LocalEntity::AaCommandStatus LocalEntityImpl<SuperClass>::convertErrorToAaCommandStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return LocalEntity::AaCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return LocalEntity::AaCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return LocalEntity::AaCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return LocalEntity::AaCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAaAecpCommand from a non-existing local entity");
			return LocalEntity::AaCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendAaAecpCommand from a non-controller entity");
			return LocalEntity::AaCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return LocalEntity::AaCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return LocalEntity::AaCommandStatus::InternalError;
}

template<class SuperClass>
LocalEntity::MvuCommandStatus LocalEntityImpl<SuperClass>::convertErrorToMvuCommandStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return LocalEntity::MvuCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return LocalEntity::MvuCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return LocalEntity::MvuCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return LocalEntity::MvuCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendMvuAecpCommand from a non-existing local entity");
			return LocalEntity::MvuCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendMvuAecpCommand from a non-controller entity");
			return LocalEntity::MvuCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::MessageNotSupported:
			AVDECC_ASSERT(false, "Trying to sendMvuAecpCommand through a ProtocolInterface not supporting it");
			return LocalEntity::MvuCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return LocalEntity::MvuCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return LocalEntity::MvuCommandStatus::InternalError;
}

template<class SuperClass>
LocalEntity::ControlStatus LocalEntityImpl<SuperClass>::convertErrorToControlStatus(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return LocalEntity::ControlStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return LocalEntity::ControlStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return LocalEntity::ControlStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return LocalEntity::ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAcmpCommand from a non-existing local entity");
			return LocalEntity::ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			return LocalEntity::ControlStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return LocalEntity::ControlStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return LocalEntity::ControlStatus::InternalError;
}

template<class SuperClass>
void LocalEntityImpl<SuperClass>::onAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept
{
	// Ignore messages not for me
	if (aecpdu.getTargetEntityID() != SuperClass::getEntityID())
		return;

	// Automatically responding to AEM Entity Available Command
	if (aecpdu.getMessageType() == protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);

		static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aem)>> s_Dispatch{
			// Entity Available
			{ protocol::AemCommandType::EntityAvailable.getValue(),
				[](protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aem)
				{
					// We are being asked if we are available, and we are! Reply that
					sendAemAecpResponse(pi, aem, protocol::AemAecpStatus::Success, nullptr, 0u);
				} },
		};

		auto const& it = s_Dispatch.find(aem.getCommandType().getValue());
		if (it != s_Dispatch.end())
		{
			invokeProtectedHandler(it->second, pi, aem);
			return;
		}
	}

	// Forward to subclass
	if (!onUnhandledAecpCommand(pi, aecpdu))
	{
		// Reflect back the command, and return a NotImplemented error code
		reflectAecpCommand(pi, aecpdu, protocol::AemAecpStatus::NotImplemented);
	}
}

template<class SuperClass>
protocol::Aecpdu::UniquePointer LocalEntityImpl<SuperClass>::createAecpdu([[maybe_unused]] protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, bool const isResponse) noexcept
{
	AVDECC_ASSERT(protocolIdentifier == la::avdecc::protocol::MvuAecpdu::ProtocolID, "Registered this class for MVU only (currently), should not get any other protocolIdentifier!!");
	return protocol::MvuAecpdu::create(isResponse);
}

template<class SuperClass>
bool LocalEntityImpl<SuperClass>::areHandledByControllerStateMachine([[maybe_unused]] protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier) const noexcept
{
	AVDECC_ASSERT(protocolIdentifier == la::avdecc::protocol::MvuAecpdu::ProtocolID, "Registered this class for MVU only (currently), should not get any other protocolIdentifier!!");
	return true;
}

template<class SuperClass>
std::uint32_t LocalEntityImpl<SuperClass>::getVuAecpCommandTimeoutMsec([[maybe_unused]] protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, protocol::VuAecpdu const& /*aecpdu*/) noexcept
{
	AVDECC_ASSERT(protocolIdentifier == la::avdecc::protocol::MvuAecpdu::ProtocolID, "Registered this class for MVU only (currently), should not get any other protocolIdentifier!!");
	return 250u;
}

template<class SuperClass>
void LocalEntityImpl<SuperClass>::onVuAecpCommand(la::avdecc::protocol::ProtocolInterface* const pi, la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, la::avdecc::protocol::VuAecpdu const& aecpdu) noexcept
{
	AVDECC_ASSERT(aecpdu.getMessageType() == protocol::AecpMessageType::VendorUniqueCommand, "onVuAecpCommand called for something else than a VendorUniqueCommand");
	AVDECC_ASSERT(protocolIdentifier == la::avdecc::protocol::MvuAecpdu::ProtocolID, "Registered this class for MVU only (currently), should not get any other protocolIdentifier!!");

	// Ignore messages not for me
	if (aecpdu.getTargetEntityID() != SuperClass::getEntityID())
		return;

	// Forward to subclass
	if (!onUnhandledAecpVuCommand(pi, protocolIdentifier, aecpdu))
	{
		// Reflect back the command, and return a NotImplemented error code (there is no "NotSupported" code for MVU)
		reflectAecpCommand(pi, aecpdu, protocol::AecpStatus::NotImplemented);
	}
}

} // namespace entity
} // namespace avdecc
} // namespace la
