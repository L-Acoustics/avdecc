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
* @file avdecc.cpp
* @author Christophe Calmejane
* @brief Global avdecc library methods (version handling, ...).
*/

#include "la/avdecc/avdecc.hpp"

#include "config.h"

namespace la
{
namespace avdecc
{
bool LA_AVDECC_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept
{
	/* Here you have to choose a compatibility mode
	*  1/ Strict mode
	*     The version of the library used to compile must be strictly equivalent to the one used at runtime.
	*  2/ Backward compatibility mode
	*     A newer version of the library used at runtime is backward compatible with an older one used to compile.<BR>
	*     When using that mode, each class must use a virtual interface, and each new version must derivate from the interface to propose new methods.
	*  3/ A combination of the 2 above, using code to determine which one depending on the passed interfaceVersion value.
	*/

	// Interface version should be strictly equivalent
	return InterfaceVersion == interfaceVersion;
}

std::string LA_AVDECC_CALL_CONVENTION getVersion() noexcept
{
	return std::string(LA_AVDECC_LIB_VERSION);
}

std::uint32_t LA_AVDECC_CALL_CONVENTION getInterfaceVersion() noexcept
{
	return InterfaceVersion;
}

CompileOptions LA_AVDECC_CALL_CONVENTION getCompileOptions() noexcept
{
	CompileOptions options{};

	for (auto const& info : getCompileOptionsInfo())
	{
		options.set(info.option);
	}

	return options;
}

std::vector<CompileOptionInfo> LA_AVDECC_CALL_CONVENTION getCompileOptionsInfo() noexcept
{
	std::vector<CompileOptionInfo> options{};

#ifdef IGNORE_INVALID_CONTROL_DATA_LENGTH
	options.emplace_back(CompileOptionInfo{ CompileOption::IgnoreInvalidControlDataLength, "IICDL", "Ignore Invalid Control Data Length" });
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH

#ifdef IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES
	options.emplace_back(CompileOptionInfo{ CompileOption::IgnoreInvalidNonSuccessAemResponses, "IINSAR", "Ignore Invalid Non Success AEM Responses" });
#endif // IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES

#ifdef ALLOW_GET_AUDIO_MAP_UNSOL
	options.emplace_back(CompileOptionInfo{ CompileOption::AllowGetAudioMapUnsol, "AGAMU", "Allow Get Audio Map Unsolicited Notifications" });
#endif // ALLOW_GET_AUDIO_MAP_UNSOL

#ifdef ALLOW_SEND_BIG_AECP_PAYLOADS
	options.emplace_back(CompileOptionInfo{ CompileOption::AllowSendBigAecpPayloads, "ASBAP", "Allow Send Big AECP payloads" });
#endif // ALLOW_SEND_BIG_AECP_PAYLOADS

#ifdef ALLOW_RECV_BIG_AECP_PAYLOADS
	options.emplace_back(CompileOptionInfo{ CompileOption::AllowRecvBigAecpPayloads, "ARBAP", "Allow Recv Big AECP payloads" });
#endif // ALLOW_RECV_BIG_AECP_PAYLOADS

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	options.emplace_back(CompileOptionInfo{ CompileOption::EnableRedundancy, "RDNCY", "Redundancy" });
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

#ifdef ENABLE_AVDECC_FEATURE_JSON
	options.emplace_back(CompileOptionInfo{ CompileOption::EnableJsonSupport, "JSN", "JSON" });
#endif // ENABLE_AVDECC_FEATURE_JSON

	return options;
}

} // namespace avdecc
} // namespace la
