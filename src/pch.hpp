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
* @file pch.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/avdecc.hpp"
#include "la/avdecc/logger.hpp"
#include "la/avdecc/memoryBuffer.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/watchDog.hpp"
#include "la/avdecc/internals/aggregateEntity.hpp"
#include "la/avdecc/internals/controllerEntity.hpp"
#include "la/avdecc/internals/endpointEntity.hpp"
#include "la/avdecc/internals/endStation.hpp"
#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include "la/avdecc/internals/entityModelTypes.hpp"
#include "la/avdecc/internals/instrumentationNotifier.hpp"
#include "la/avdecc/internals/logItems.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/internals/protocolAcmpdu.hpp"
#include "la/avdecc/internals/protocolAdpdu.hpp"
#include "la/avdecc/internals/protocolAecpdu.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAvtpdu.hpp"
#include "la/avdecc/internals/protocolDefines.hpp"
#include "la/avdecc/internals/protocolInterface.hpp"
#include "la/avdecc/internals/protocolMvuAecpdu.hpp"
#include "la/avdecc/internals/protocolVuAecpdu.hpp"
#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/streamFormatInfo.hpp"
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include "la/avdecc/internals/jsonSerialization.hpp"
#	include "la/avdecc/internals/jsonTypes.hpp"
#endif // ENABLE_AVDECC_FEATURE_JSON

#ifdef HAVE_PROTOCOL_INTERFACE_PCAP
// We must include pcap now, because of winsock.h and windows.h
#	include <pcap.h>
#endif // HAVE_PROTOCOL_INTERFACE_PCAP

#include "config.h"
#include "logHelper.hpp"
#include "endStationImpl.hpp"
#include "entity/aggregateEntityImpl.hpp"
#include "entity/controllerCapabilityDelegate.hpp"
#include "entity/controllerEntityImpl.hpp"
#include "entity/endpointCapabilityDelegate.hpp"
#include "entity/endpointEntityImpl.hpp"
#include "entity/entityImpl.hpp"
#include "protocol/protocolAemPayloads.hpp"
#include "protocol/protocolAemPayloadSizes.hpp"
#include "protocol/protocolMvuPayloads.hpp"
#include "protocol/protocolMvuPayloadSizes.hpp"
#include "stateMachine/advertiseStateMachine.hpp"
#include "stateMachine/commandStateMachine.hpp"
#include "stateMachine/discoveryStateMachine.hpp"
#include "stateMachine/protocolInterfaceDelegate.hpp"
#include "stateMachine/stateMachineManager.hpp"
#include "stateMachine/stateMachineManager.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
