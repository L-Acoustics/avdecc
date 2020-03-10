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
* @file protocolAvtpdu_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/protocolAvtpdu.hpp>
#include <la/avdecc/internals/protocolAecpdu.hpp>
#include <la/avdecc/internals/protocolAemAecpdu.hpp>

#include <gtest/gtest.h>

/***********************************************************/
/* AEM tests                                               */
/***********************************************************/

TEST(Aem, SerializeFrame)
{
	auto frame = la::avdecc::protocol::AemAecpdu::create(false);
	auto& aem = static_cast<la::avdecc::protocol::AemAecpdu&>(*frame);

	// Set AECP fields
	aem.setStatus(la::avdecc::protocol::AecpStatus::NotImplemented);
	aem.setTargetEntityID(la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier());
	aem.setControllerEntityID(la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier());
	aem.setSequenceID(0x15);
	// Set AEM fields
	aem.setUnsolicited(true);
	aem.setCommandType(la::avdecc::protocol::AemCommandType::EntityAvailable);

	// Serialize AECP frame only (not AVTP nor Eth2)
	la::avdecc::protocol::SerializationBuffer buffer;
	frame->serialize(buffer);

#pragma message("TODO: Check raw buffer values")
}
