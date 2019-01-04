/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file controllerStateMachine_tests.cpp
* @author Christophe Calmejane
*/

// Internal API
#include "stateMachine/controllerStateMachine.hpp"

#include <gtest/gtest.h>

TEST(ControllerStateMachine, InvalidDelegate)
{
	EXPECT_THROW(la::avdecc::protocol::stateMachine::ControllerStateMachine(nullptr, nullptr);, la::avdecc::Exception);
}
