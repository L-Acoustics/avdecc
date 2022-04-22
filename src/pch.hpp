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
* @file pch.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/avdecc.hpp"
#include "la/avdecc/executor.hpp"
#include "la/avdecc/logger.hpp"
#include "la/avdecc/memoryBuffer.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/watchDog.hpp"
#include "la/avdecc/internals/aggregateEntity.hpp"
#include "la/avdecc/internals/controllerEntity.hpp"
#include "la/avdecc/internals/endian.hpp"
#include "la/avdecc/internals/endStation.hpp"
#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/internals/entityAddressAccessTypes.hpp"
#include "la/avdecc/internals/entityEnums.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include "la/avdecc/internals/entityModelControlValues.hpp"
#include "la/avdecc/internals/entityModelControlValuesTraits.hpp"
#include "la/avdecc/internals/entityModelTree.hpp"
#include "la/avdecc/internals/entityModelTreeCommon.hpp"
#include "la/avdecc/internals/entityModelTreeDynamic.hpp"
#include "la/avdecc/internals/entityModelTreeStatic.hpp"
#include "la/avdecc/internals/entityModelTypes.hpp"
#include "la/avdecc/internals/exception.hpp"
#include "la/avdecc/internals/exports.hpp"
#include "la/avdecc/internals/instrumentationNotifier.hpp"
#include "la/avdecc/internals/jsonSerialization.hpp"
#include "la/avdecc/internals/logItems.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/internals/protocolAcmpdu.hpp"
#include "la/avdecc/internals/protocolAdpdu.hpp"
#include "la/avdecc/internals/protocolAecpdu.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAemPayloadSizes.hpp"
#include "la/avdecc/internals/protocolAvtpdu.hpp"
#include "la/avdecc/internals/protocolDefines.hpp"
#include "la/avdecc/internals/protocolMvuAecpdu.hpp"
#include "la/avdecc/internals/protocolMvuPayloadSizes.hpp"
#include "la/avdecc/internals/protocolVuAecpdu.hpp"
#include "la/avdecc/internals/protocolInterface.hpp"
#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/streamFormatInfo.hpp"
#include "la/avdecc/internals/uniqueIdentifier.hpp"
#ifdef ENABLE_AVDECC_FEATURE_JSON
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
#include "entity/entityImpl.hpp"
#include "protocol/protocolAemControlValuesPayloads.hpp"
#include "protocol/protocolAemPayloads.hpp"
#include "protocol/protocolMvuPayloads.hpp"
#include "stateMachine/advertiseStateMachine.hpp"
#include "stateMachine/commandStateMachine.hpp"
#include "stateMachine/discoveryStateMachine.hpp"
#include "stateMachine/protocolInterfaceDelegate.hpp"
#include "stateMachine/stateMachineManager.hpp"
#include "stateMachine/stateMachineManager.hpp"

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#if defined(ENABLE_AVDECC_CUSTOM_ANY)
#	include "la/avdecc/internals/any.hpp"
#else // !ENABLE_AVDECC_CUSTOM_ANY
#	include <any>
#endif // ENABLE_AVDECC_CUSTOM_ANY

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
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <version>
