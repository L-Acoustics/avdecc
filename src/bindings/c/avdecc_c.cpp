/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file avdecc_c.cpp
* @author Christophe Calmejane
* @brief C bindings over LA_avdecc library.
*/

#include <la/avdecc/internals/uniqueIdentifier.hpp>
#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/avdecc.h"
#include "utils.hpp"
#include "config.h"
#include <cstdlib>

LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_isCompatibleWithInterfaceVersion(avdecc_interface_version_t const interfaceVersion)
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
	return static_cast<avdecc_bool_t>(LA_AVDECC_InterfaceVersion == interfaceVersion);
}

LA_AVDECC_BINDINGS_C_API avdecc_const_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getVersion()
{
	return LA_AVDECC_BINDINGS_C_VERSION;
}

LA_AVDECC_BINDINGS_C_API avdecc_interface_version_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getInterfaceVersion()
{
	return LA_AVDECC_InterfaceVersion;
}

LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_initialize() {}

LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_uninitialize() {}

LA_AVDECC_BINDINGS_C_API avdecc_unique_identifier_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getNullUniqueIdentifier()
{
	return la::avdecc::UniqueIdentifier::getNullUniqueIdentifier();
}

LA_AVDECC_BINDINGS_C_API avdecc_unique_identifier_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getUninitializedUniqueIdentifier()
{
	return la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier();
}

LA_AVDECC_BINDINGS_C_API avdecc_unique_identifier_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_generateEID(avdecc_mac_address_cp const address, unsigned short const progID)
{
	try
	{
		return la::avdecc::entity::Entity::generateEID(la::avdecc::bindings::fromCToCpp::make_macAddress(address), progID, false);
	}
	catch (...)
	{
		return la::avdecc::UniqueIdentifier::getNullUniqueIdentifier();
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_entity_model_descriptor_index_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getGlobalAvbInterfaceIndex()
{
	return la::avdecc::entity::Entity::GlobalAvbInterfaceIndex;
}

LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_freeString(avdecc_string_t str)
{
	if (str != nullptr)
		std::free(str);
}
