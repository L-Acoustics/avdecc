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

#include <gtest/gtest.h>
#include "controller/avdeccControlledEntityImpl.hpp"

TEST(X, X)
{
	auto const e{ la::avdecc::entity::Entity{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::networkInterface::MacAddress{}, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities::None, 0, la::avdecc::entity::ListenerCapabilities::None, la::avdecc::entity::ControllerCapabilities::None, 0, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() } };
	la::avdecc::controller::ControlledEntityImpl entity{ e };

	entity.getEntityNodeStaticModel();

	auto& res = entity.getNodeStaticModel(0, la::avdecc::entity::model::AudioUnitIndex(0), &la::avdecc::controller::model::ConfigurationStaticTree::audioUnitStaticModels);
}
