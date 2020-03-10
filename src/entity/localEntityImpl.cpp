/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file localEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/entity.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
/* ************************************************************************** */
/* Utility methods                                                            */
/* ************************************************************************** */
std::string LA_AVDECC_CALL_CONVENTION LocalEntity::statusToString(AemCommandStatus const status)
{
	switch (status)
	{
		// AVDECC Error Codes
		case AemCommandStatus::Success:
			return "Success.";
		case AemCommandStatus::NotImplemented:
			return "The AVDECC Entity does not support the command type.";
		case AemCommandStatus::NoSuchDescriptor:
			return "A descriptor with the descriptor_type and descriptor_index specified does not exist.";
		case AemCommandStatus::LockedByOther:
			return "The AVDECC Entity has been locked by another AVDECC Controller.";
		case AemCommandStatus::AcquiredByOther:
			return "The AVDECC Entity has been acquired by another AVDECC Controller.";
		case AemCommandStatus::NotAuthenticated:
			return "The AVDECC Controller is not authenticated with the AVDECC Entity.";
		case AemCommandStatus::AuthenticationDisabled:
			return "The AVDECC Controller is trying to use an authentication command when authentication isn't enable on the AVDECC Entity.";
		case AemCommandStatus::BadArguments:
			return "One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity (unsupported, incorrect combination, etc.).";
		case AemCommandStatus::NoResources:
			return "The AVDECC Entity cannot complete the command because it does not have the resources to support it.";
		case AemCommandStatus::InProgress:
			AVDECC_ASSERT(false, "Should not happen");
			return "The AVDECC Entity is processing the command and will send a second response at a later time with the result of the command.";
		case AemCommandStatus::EntityMisbehaving:
			return "The AVDECC Entity generated an internal error while trying to process the command.";
		case AemCommandStatus::NotSupported:
			return "The command is implemented but the target of the command is not supported. For example trying to set the value of a read - only Control.";
		case AemCommandStatus::StreamIsRunning:
			return "The Stream is currently streaming and the command is one which cannot be executed on an Active Stream.";
		// Library Error Codes
		case AemCommandStatus::NetworkError:
			return "Network error.";
		case AemCommandStatus::ProtocolError:
			return "Protocol error.";
		case AemCommandStatus::TimedOut:
			return "Command timed out.";
		case AemCommandStatus::UnknownEntity:
			return "Unknown entity.";
		case AemCommandStatus::InternalError:
			return "Internal error.";
		default:
			AVDECC_ASSERT(false, "Unhandled status");
			return "Unknown status.";
	}
}

std::string LA_AVDECC_CALL_CONVENTION LocalEntity::statusToString(AaCommandStatus const status)
{
	switch (status)
	{
		// AVDECC Error Codes
		case AaCommandStatus::Success:
			return "Success.";
		case AaCommandStatus::NotImplemented:
			return "The AVDECC Entity does not support the command type.";
		case AaCommandStatus::AddressTooLow:
			return "The value in the address field is below the start of the memory map.";
		case AaCommandStatus::AddressTooHigh:
			return "The value in the address field is above the end of the memory map.";
		case AaCommandStatus::AddressInvalid:
			return "The value in the address field is within the memory map but is part of an invalid region.";
		case AaCommandStatus::TlvInvalid:
			return "One or more of the TLVs were invalid. No TLVs have been processed.";
		case AaCommandStatus::DataInvalid:
			return "The data for writing is invalid.";
		case AaCommandStatus::Unsupported:
			return "A requested action was unsupported. Typically used when an unknown EXECUTE was encountered or if EXECUTE is not supported.";
		// Library Error Codes
		case AaCommandStatus::Aborted:
			return "Operation aborted.";
		case AaCommandStatus::NetworkError:
			return "Network error.";
		case AaCommandStatus::ProtocolError:
			return "Protocol error.";
		case AaCommandStatus::TimedOut:
			return "Command timed out.";
		case AaCommandStatus::UnknownEntity:
			return "Unknown entity.";
		case AaCommandStatus::InternalError:
			return "Internal error.";
		default:
			AVDECC_ASSERT(false, "Unhandled status");
			return "Unknown status.";
	}
}

std::string LA_AVDECC_CALL_CONVENTION LocalEntity::statusToString(MvuCommandStatus const status)
{
	switch (status)
	{
		// Milan Vendor Unique Error Codes
		case MvuCommandStatus::Success:
			return "Success.";
		case MvuCommandStatus::NotImplemented:
			return "The AVDECC Entity does not support the command type.";
		case MvuCommandStatus::BadArguments:
			return "One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity (unsupported, incorrect combination, etc.).";
		// Library Error Codes
		case MvuCommandStatus::NetworkError:
			return "Network error.";
		case MvuCommandStatus::ProtocolError:
			return "Protocol error.";
		case MvuCommandStatus::TimedOut:
			return "Command timed out.";
		case MvuCommandStatus::UnknownEntity:
			return "Unknown entity.";
		case MvuCommandStatus::InternalError:
			return "Internal error.";
		default:
			AVDECC_ASSERT(false, "Unhandled status");
			return "Unknown status.";
	}
}

std::string LA_AVDECC_CALL_CONVENTION LocalEntity::statusToString(ControlStatus const status)
{
	switch (status)
	{
		// AVDECC Error Codes
		case ControlStatus::Success:
			return "Success";
		case ControlStatus::ListenerUnknownID:
			return "Listener does not have the specified unique identifier";
		case ControlStatus::TalkerUnknownID:
			return "Talker does not have the specified unique identifier";
		case ControlStatus::TalkerDestMacFail:
			return "Talker could not allocate a destination MAC for the Stream";
		case ControlStatus::TalkerNoStreamIndex:
			return "Talker does not have an available Stream index for the Stream";
		case ControlStatus::TalkerNoBandwidth:
			return "Talker could not allocate bandwidth for the Stream";
		case ControlStatus::TalkerExclusive:
			return "Talker already has an established Stream and only supports one Listener";
		case ControlStatus::ListenerTalkerTimeout:
			return "Listener had timeout for all retries when trying to send command to Talker";
		case ControlStatus::ListenerExclusive:
			return "The AVDECC Listener already has an established connection to a Stream";
		case ControlStatus::StateUnavailable:
			return "Could not get the state from the AVDECC Entity";
		case ControlStatus::NotConnected:
			return "Trying to disconnect when not connected or not connected to the AVDECC Talker specified";
		case ControlStatus::NoSuchConnection:
			return "Trying to obtain connection info for an AVDECC Talker connection which does not exist";
		case ControlStatus::CouldNotSendMessage:
			return "The AVDECC Listener failed to send the message to the AVDECC Talker";
		case ControlStatus::TalkerMisbehaving:
			return "Talker was unable to complete the command because an internal error occurred";
		case ControlStatus::ListenerMisbehaving:
			return "Listener was unable to complete the command because an internal error occurred";
		// Reserved
		case ControlStatus::ControllerNotAuthorized:
			return "The AVDECC Controller with the specified Entity ID is not authorized to change Stream connections";
		case ControlStatus::IncompatibleRequest:
			return "The AVDECC Listener is trying to connect to an AVDECC Talker that is already streaming with a different traffic class, etc. or does not support the requested traffic class";
		case ControlStatus::NotSupported:
			return "The command is not supported";
		// Library Error Codes
		case ControlStatus::NetworkError:
			return "Network error";
		case ControlStatus::ProtocolError:
			return "Protocol error";
		case ControlStatus::TimedOut:
			return "Control timed out";
		case ControlStatus::UnknownEntity:
			return "Unknown entity";
		case ControlStatus::InternalError:
			return "Internal error";
		default:
			AVDECC_ASSERT(false, "Unhandled status");
			return "Unknown status";
	}
}


} // namespace entity
} // namespace avdecc
} // namespace la
