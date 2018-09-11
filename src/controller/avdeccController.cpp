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

/**
* @file avdeccController.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerImpl.hpp"
#include "config.h"

namespace la
{
namespace avdecc
{
namespace controller
{

bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept
{
	/* Here you have to choose a compatibility mode
	*  1/ Strict mode
	*     The version of the library used to compile must be strictly equivalent to the one used at runtime.
	*  2/ Backward compatibility mode
	*     A newer version of the library used at runtime is backward compatible with an older one used to compile.
	*     When using that mode, each class must use a virtual interface, and each new version must derivate from the interface to propose new methods.
	*  3/ A combination of the 2 above, using code to determine which one depending on the passed interfaceVersion value.
	*/

	// Interface versions should be strictly equivalent
	return InterfaceVersion == interfaceVersion;
}

std::string LA_AVDECC_CONTROLLER_CALL_CONVENTION getVersion() noexcept
{
	return std::string(LA_AVDECC_CONTROLLER_LIB_VERSION);
}

std::uint32_t LA_AVDECC_CONTROLLER_CALL_CONVENTION getInterfaceVersion() noexcept
{
	return InterfaceVersion;
}

CompileOptions LA_AVDECC_CONTROLLER_CALL_CONVENTION getCompileOptions() noexcept
{
	CompileOptions options{};

	for (auto const& info : getCompileOptionsInfo())
	{
		options.set(info.option);
	}

	return options;
}

std::vector<CompileOptionInfo> LA_AVDECC_CONTROLLER_CALL_CONVENTION getCompileOptionsInfo() noexcept
{
	std::vector<CompileOptionInfo> options{};

#ifdef IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
	options.emplace_back(CompileOptionInfo{ CompileOption::IgnoreNeitherStaticNorDynamicMappings, "INSNDM", "Ignore Neither Static Nor Dynamic Mappings" });
#endif // IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	options.emplace_back(CompileOptionInfo{ CompileOption::EnableRedundancy, "RDNCY", "Redundancy" });
#ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
	options.emplace_back(CompileOptionInfo{ CompileOption::Strict2018Redundancy, "RDNCY2018", "Strict 2018 Redundancy" });
#endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	return options;
}

/* ************************************************************ */
/* Controller class definition                                  */
/* ************************************************************ */
Controller* LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::createRawController(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale)
{
	return new ControllerImpl(protocolInterfaceType, interfaceName, progID, entityModelID, preferedLocale);
}

} // namespace controller
} // namespace avdecc
} // namespace la
