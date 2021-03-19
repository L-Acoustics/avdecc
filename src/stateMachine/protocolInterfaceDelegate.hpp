/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file protocolInterfaceDelegate.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/protocolInterface.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace stateMachine
{
class ProtocolInterfaceDelegate
{
public:
	virtual ~ProtocolInterfaceDelegate() noexcept = default;

	/* **** AECP notifications **** */
	virtual void onAecpCommand(la::avdecc::protocol::Aecpdu const& aecpdu) noexcept = 0;
	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(la::avdecc::protocol::Acmpdu const& acmpdu) noexcept = 0;
	virtual void onAcmpResponse(la::avdecc::protocol::Acmpdu const& acmpdu) noexcept = 0;
	/* **** Sending methods **** */
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Adpdu const& adpdu) const noexcept = 0;
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Aecpdu const& aecpdu) const noexcept = 0;
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Acmpdu const& acmpdu) const noexcept = 0;
	/* *** Other methods **** */
	virtual std::uint32_t getVuAecpCommandTimeoutMsec(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, la::avdecc::protocol::VuAecpdu const& aecpdu) const noexcept = 0;
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
