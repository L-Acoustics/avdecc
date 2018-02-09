/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
* @file avdeccControlledEntityStaticModel.hpp
* @author Christophe Calmejane
*/

#include <la/avdecc/avdecc.hpp>
#include <map>

namespace la
{
namespace avdecc
{
namespace controller
{
namespace model
{

struct ConfigurationStaticModel
{
	entity::model::ConfigurationDescriptor configurationDescriptor{};
	entity::model::LocaleDescriptor const* selectedLocaleDescriptor{ nullptr }; /** Selected locale */
	std::map<entity::model::StringsIndex, entity::model::AvdeccFixedString> localizedStrings{}; /** Localized strings for selected locale */

	std::map<entity::model::AudioUnitIndex, entity::model::AudioUnitDescriptor> audioUnitDescriptors{};
	std::map<entity::model::StreamIndex, entity::model::StreamDescriptor> streamInputDescriptors{};
	std::map<entity::model::StreamIndex, entity::model::StreamDescriptor> streamOutputDescriptors{};
	//std::map<entity::model::JackIndex, entity::model::JackDescriptor> jackInputDescriptors{};
	//std::map<entity::model::JackIndex, entity::model::JackDescriptor> jackOutputDescriptors{};
	std::map<entity::model::AvbInterfaceIndex, entity::model::AvbInterfaceDescriptor> avbInterfaceDescriptors{};
	std::map<entity::model::ClockSourceIndex, entity::model::ClockSourceDescriptor> clockSourceDescriptors{};
	//std::map<entity::model::MemoryObjectIndex, entity::model::MemoryObjectDescriptor> memoryObjectDescriptors{};
	std::map<entity::model::LocaleIndex, entity::model::LocaleDescriptor> localeDescriptors{};
	std::map<entity::model::StringsIndex, entity::model::StringsDescriptor> stringsDescriptors{};
	std::map<entity::model::StreamPortIndex, entity::model::StreamPortDescriptor> streamPortInputDescriptors{};
	std::map<entity::model::StreamPortIndex, entity::model::StreamPortDescriptor> streamPortOutputDescriptors{};
	//std::map<entity::model::ExternalPortIndex, entity::model::ExternalPortDescriptor> externalPortInputDescriptors{};
	//std::map<entity::model::ExternalPortIndex, entity::model::ExternalPortDescriptor> externalPortOutputDescriptors{};
	//std::map<entity::model::InternalPortIndex, entity::model::InternalPortDescriptor> internalPortInputDescriptors{};
	//std::map<entity::model::InternalPortIndex, entity::model::InternalPortDescriptor> internalPortOutputDescriptors{};
	std::map<entity::model::ClusterIndex, entity::model::AudioClusterDescriptor> audioClusterDescriptors{};
	std::map<entity::model::MapIndex, entity::model::AudioMapDescriptor> audioMapDescriptors{};
	std::map<entity::model::ClockDomainIndex, entity::model::ClockDomainDescriptor> clockDomainDescriptors{};
};

struct EntityStaticModel
{
	entity::model::EntityDescriptor entityDescriptor{};
	std::map<entity::model::ConfigurationIndex, ConfigurationStaticModel> configurationStaticModels{};
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
