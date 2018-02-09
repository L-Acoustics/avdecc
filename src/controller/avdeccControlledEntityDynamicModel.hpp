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
* @file avdeccControlledEntityDynamicModel.hpp
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

//struct AvbInterfaceDynamicModel
//{
//	entity::model::AvbInfo
//	entity::model::AsPath
//};

struct StreamDynamicModel
{
	entity::model::StreamConnectedState connectedState{ getUninitializedIdentifier() };
	entity::model::StreamInfo streamInfo{};
	bool isRunning{ true };
};

struct StreamPortDynamicModel
{
	entity::model::AudioMappings dynamicAudioMap{};
};

struct ConfigurationDynamicModel
{
	std::map<entity::model::StreamIndex, StreamDynamicModel> streamInputDynamicModels{};
	std::map<entity::model::StreamIndex, StreamDynamicModel> streamOutputDynamicModels{};
	std::map<entity::model::StreamPortIndex, StreamPortDynamicModel> streamPortInputDynamicModels{};
	std::map<entity::model::StreamPortIndex, StreamPortDynamicModel> streamPortOutputDynamicModels{};
};

struct EntityDynamicModel
{
	std::map<entity::model::ConfigurationIndex, ConfigurationDynamicModel> configurationDynamicModels{};
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
