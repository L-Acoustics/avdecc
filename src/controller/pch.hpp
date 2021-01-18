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

#include "la/avdecc/memoryBuffer.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/controller/avdeccController.hpp"
#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
#include "la/avdecc/controller/internals/logItems.hpp"

#include "config.h"
#include "avdeccControlledEntityImpl.hpp"
#include "avdeccControllerImpl.hpp"
#include "avdeccControllerLogHelper.hpp"
#include "avdeccEntityModelCache.hpp"
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include "avdeccControlledEntityJsonSerializer.hpp"
#	include "avdeccControllerJsonTypes.hpp"
#endif // ENABLE_AVDECC_FEATURE_JSON

#include <la/avdecc/internals/entityModelTree.hpp>
#include <la/avdecc/internals/entityModelControlValues.hpp>
#include <la/avdecc/internals/serialization.hpp>
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include <la/avdecc/internals/jsonSerialization.hpp>
#	include <la/avdecc/internals/jsonTypes.hpp>
#	include <nlohmann/json.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <set>
