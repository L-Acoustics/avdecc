/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file avdecc.hpp
* @author Christophe Calmejane
* @brief Main avdecc library header file.
*/

#pragma once

/** Protocol defines */
#include "internals/protocolDefines.hpp"

/** Entity model definition */
#include "internals/entityModel.hpp"

/** Entity model tree definition */
#include "internals/entityModelTree.hpp"

/** ProtocolInterface definition */
#include "internals/protocolInterface.hpp"

/** Protocols definition */
#include "internals/protocolAvtpdu.hpp"
#include "internals/protocolAcmpdu.hpp"
#include "internals/protocolAdpdu.hpp"
#include "internals/protocolAecpdu.hpp"
#include "internals/protocolAaAecpdu.hpp"
#include "internals/protocolAemAecpdu.hpp"

/** EndStation definition */
#include "internals/endStation.hpp"

/** Controller entity type definition */
#include "internals/controllerEntity.hpp"

/** Symbols export definition */
#include "internals/exports.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace la
{
namespace avdecc
{
/**
* Interface version of the library, used to check for compatibility between the version used to compile and the runtime version.<BR>
* Everytime the interface changes (what is visible from the user) you increase the InterfaceVersion value.<BR>
* A change in the visible interface is any modification in a public header file except a change in a private non-virtual method
* (either added, removed or signature modification).
* Any other change (including templates, inline methods, defines, typedefs, ...) are considered a modification of the interface.
*/
constexpr std::uint32_t InterfaceVersion = 300;

/**
* @brief Checks if the library is compatible with specified interface version.
* @param[in] interfaceVersion The interface version to check for compatibility.
* @return True if the library is compatible.
* @note If the library is not compatible, the application should no longer use the library.<BR>
*       When using the avdecc shared library, you must call this version to check the compatibility between the compiled and the loaded version.
*/
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept;

/**
* @brief Gets the avdecc library version.
* @details Returns the avdecc library version.
* @return A string representing the library version.
*/
LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION getVersion() noexcept;

/**
* @brief Gets the avdecc shared library interface version.
* @details Returns the avdecc shared library interface version.
* @return The interface version.
*/
LA_AVDECC_API std::uint32_t LA_AVDECC_CALL_CONVENTION getInterfaceVersion() noexcept;

enum class CompileOption : std::uint32_t
{
	None = 0,
	IgnoreInvalidControlDataLength = 1u << 0,
	IgnoreInvalidNonSuccessAemResponses = 1u << 1,
	AllowGetAudioMapUnsol = 1u << 2,
	AllowSendBigAecpPayloads = 1u << 3,
	AllowRecvBigAecpPayloads = 1u << 4,
	EnableRedundancy = 1u << 15,
	EnableJsonSupport = 1u << 16,
};
using CompileOptions = utils::EnumBitfield<CompileOption>;

struct CompileOptionInfo
{
	CompileOption option{ CompileOption::None };
	std::string shortName{};
	std::string longName{};
};

/**
* @brief Gets the avdecc library compile options.
* @details Returns the avdecc library compile options.
* @return The compile options.
*/
LA_AVDECC_API CompileOptions LA_AVDECC_CALL_CONVENTION getCompileOptions() noexcept;

/**
* @brief Gets the avdecc library compile options info.
* @details Returns the avdecc library compile options info.
* @return The compile options info.
*/
LA_AVDECC_API std::vector<CompileOptionInfo> LA_AVDECC_CALL_CONVENTION getCompileOptionsInfo() noexcept;

} // namespace avdecc
} // namespace la
