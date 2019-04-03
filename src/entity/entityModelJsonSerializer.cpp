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
* @file entityModelJsonSerializer.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/jsonSerialization.hpp"
#include "la/avdecc/internals/jsonTypes.hpp"

using json = nlohmann::json;

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
namespace jsonSerializer
{
struct Context
{
	AudioUnitIndex nextExpectedAudioUnitIndex{ 0u };
	StreamIndex nextExpectedStreamInputIndex{ 0u };
	StreamIndex nextExpectedStreamOutputIndex{ 0u };
	AvbInterfaceIndex nextExpectedAvbInterfaceIndex{ 0u };
	ClockSourceIndex nextExpectedClockSourceIndex{ 0u };
	MemoryObjectIndex nextExpectedMemoryObjectIndex{ 0u };
	LocaleIndex nextExpectedLocaleIndex{ 0u };
	StreamPortIndex nextExpectedStreamPortInputIndex{ 0u };
	StreamPortIndex nextExpectedStreamPortOutputIndex{ 0u };
	ClusterIndex nextExpectedAudioClusterIndex{ 0u };
	MapIndex nextExpectedAudioMapIndex{ 0u };
	ClockDomainIndex nextExpectedClockDomainIndex{ 0u };
};

template<bool hasDynamicModel = true, typename FieldPointer>
nlohmann::json dumpLeafModels(ConfigurationTree const& configTree, SerializationFlags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex& nextExpectedIndex, std::string const& descriptorName, DescriptorIndex const baseIndex, size_t const numberOfIndexes)
{
	auto objects = json{};

	for (auto descriptorIndexCounter = DescriptorIndex(0); descriptorIndexCounter < numberOfIndexes; ++descriptorIndexCounter)
	//for (auto const& [descriptorIndex, models] : (configTree.*Field))
	{
		auto const descriptorIndex = DescriptorIndex(descriptorIndexCounter + baseIndex);
		if (descriptorIndex != nextExpectedIndex)
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(descriptorIndex) + " but expected " + std::to_string(nextExpectedIndex) };
		}
		++nextExpectedIndex;

		auto const modelsIt = (configTree.*Field).find(descriptorIndex);
		if (modelsIt == (configTree.*Field).end())
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(descriptorIndex) + " (out of range)" };
		}
		auto const& models = modelsIt->second;

		auto object = json{};

		// Dump Static model
		if (flags.test(SerializationFlag::SerializeStaticModel))
		{
			// Dump Descriptor Model
			object[keyName::Node_StaticInformation] = models.staticModel;
		}

		// Dump Dynamic model
		if constexpr (hasDynamicModel)
		{
			if (flags.test(SerializationFlag::SerializeDynamicModel))
			{
				// Dump Descriptor Model
				object[keyName::Node_DynamicInformation] = models.dynamicModel;
			}
		}

		// Dump informative DescriptorIndex
		object[model::keyName::Node_Informative_Index] = descriptorIndex;

		objects.push_back(std::move(object));
	}

	return objects;
}

template<typename FieldPointer>
nlohmann::json dumpStringsModels(ConfigurationTree const& configTree, SerializationFlags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex const baseStrings, std::uint16_t const numberOfStrings)
{
	auto strings = json{};

	for (auto stringsIndexCounter = StringsIndex(0); stringsIndexCounter < numberOfStrings; ++stringsIndexCounter)
	{
		auto const stringsIndex = StringsIndex(stringsIndexCounter + baseStrings);
		auto const stringsModelsIt = (configTree.*Field).find(stringsIndex);
		if (stringsModelsIt == (configTree.*Field).end())
		{
			// Don't throw if Strings not found (if it's the first of the range), it was probably not loaded
			if (stringsIndexCounter == 0)
			{
				break;
			}
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid Strings Descriptor Index: " + std::to_string(stringsIndex) + " (out of range)" };
		}
		auto const& stringsModels = stringsModelsIt->second;

		auto string = json{};

		// Dump Static model
		if (flags.test(SerializationFlag::SerializeStaticModel))
		{
			// Dump Strings Descriptor Model
			string[keyName::Node_StaticInformation] = stringsModels.staticModel;
		}

		// Dump informative DescriptorIndex
		string[model::keyName::Node_Informative_Index] = stringsIndex;

		strings.push_back(std::move(string));
	}

	return strings;
}

template<typename FieldPointer>
nlohmann::json dumpStreamPortModels(Context& c, ConfigurationTree const& configTree, SerializationFlags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex& nextExpectedIndex, DescriptorIndex const baseStreamPort, std::uint16_t const numberOfStreamPorts)
{
	auto streamPorts = json{};

	for (auto streamPortIndexCounter = StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
	{
		auto const streamPortIndex = StreamPortIndex(streamPortIndexCounter + baseStreamPort);
		if (streamPortIndex != nextExpectedIndex)
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid StreamPort Descriptor Index: " + std::to_string(streamPortIndex) + " but expected " + std::to_string(nextExpectedIndex) };
		}
		++nextExpectedIndex;

		auto const streamPortModelsIt = (configTree.*Field).find(streamPortIndex);
		if (streamPortModelsIt == (configTree.*Field).end())
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid StreamPort Descriptor Index: " + std::to_string(streamPortIndex) + " (out of range)" };
		}
		auto const& streamPortModels = streamPortModelsIt->second;

		auto streamPort = json{};

		// Dump Static model
		auto const& staticModel = streamPortModels.staticModel;
		if (flags.test(SerializationFlag::SerializeStaticModel))
		{
			// Dump StreamPort Descriptor Model
			streamPort[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(SerializationFlag::SerializeDynamicModel))
		{
			// Dump StreamPort Descriptor Model
			streamPort[keyName::Node_DynamicInformation] = streamPortModels.dynamicModel;
		}

		// Dump AudioClusters
		streamPort[keyName::NodeName_AudioClusterDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::audioClusterModels, c.nextExpectedAudioClusterIndex, "AudioCluster", staticModel.baseCluster, staticModel.numberOfClusters);

		// Dump AudioMaps
		streamPort[keyName::NodeName_AudioMapDescriptors] = dumpLeafModels<false>(configTree, flags, &ConfigurationTree::audioMapModels, c.nextExpectedAudioMapIndex, "AudioMap", staticModel.baseMap, staticModel.numberOfMaps);

		// Dump informative DescriptorIndex
		streamPort[model::keyName::Node_Informative_Index] = streamPortIndex;

		streamPorts.push_back(std::move(streamPort));
	}

	return streamPorts;
}

nlohmann::json dumpAudioUnitModels(Context& c, ConfigurationTree const& configTree, SerializationFlags const flags)
{
	auto audioUnits = json{};

	for (auto const& [audioUnitIndex, audioUnitModels] : configTree.audioUnitModels)
	{
		if (audioUnitIndex != c.nextExpectedAudioUnitIndex)
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid AudioUnit Descriptor Index: " + std::to_string(audioUnitIndex) + " but expected " + std::to_string(c.nextExpectedAudioUnitIndex) };
		}
		++c.nextExpectedAudioUnitIndex;

		auto audioUnit = json{};

		// Dump Static model
		auto const& staticModel = audioUnitModels.staticModel;
		if (flags.test(SerializationFlag::SerializeStaticModel))
		{
			// Dump AudioUnit Descriptor Model
			audioUnit[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(SerializationFlag::SerializeDynamicModel))
		{
			// Dump AudioUnit Descriptor Model
			audioUnit[keyName::Node_DynamicInformation] = audioUnitModels.dynamicModel;
		}

		// Dump StreamPortInputs
		audioUnit[keyName::NodeName_StreamPortInputDescriptors] = dumpStreamPortModels(c, configTree, flags, &ConfigurationTree::streamPortInputModels, c.nextExpectedStreamPortInputIndex, staticModel.baseStreamInputPort, staticModel.numberOfStreamInputPorts);

		// Dump StreamPortOutputs
		audioUnit[keyName::NodeName_StreamPortOutputDescriptors] = dumpStreamPortModels(c, configTree, flags, &ConfigurationTree::streamPortOutputModels, c.nextExpectedStreamPortOutputIndex, staticModel.baseStreamOutputPort, staticModel.numberOfStreamOutputPorts);

		// Dump informative DescriptorIndex
		audioUnit[model::keyName::Node_Informative_Index] = audioUnitIndex;

		audioUnits.push_back(std::move(audioUnit));
	}

	return audioUnits;
}

nlohmann::json dumpLocaleModels(Context& c, ConfigurationTree const& configTree, SerializationFlags const flags)
{
	auto locales = json{};

	for (auto const& [localeIndex, localeModels] : configTree.localeModels)
	{
		if (localeIndex != c.nextExpectedLocaleIndex)
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid Locale Descriptor Index: " + std::to_string(localeIndex) + " but expected " + std::to_string(c.nextExpectedLocaleIndex) };
		}
		++c.nextExpectedLocaleIndex;

		auto locale = json{};

		// Dump Static model
		auto const& staticModel = localeModels.staticModel;
		if (flags.test(SerializationFlag::SerializeStaticModel))
		{
			// Dump Locale Descriptor Model
			locale[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Strings
		locale[keyName::NodeName_StringsDescriptors] = dumpStringsModels(configTree, flags, &ConfigurationTree::stringsModels, staticModel.baseStringDescriptorIndex, staticModel.numberOfStringDescriptors);

		// Dump informative DescriptorIndex
		locale[model::keyName::Node_Informative_Index] = localeIndex;

		locales.push_back(std::move(locale));
	}

	return locales;
}

nlohmann::json dumpConfigurationTrees(std::map<ConfigurationIndex, ConfigurationTree> const& configTrees, SerializationFlags const flags)
{
	auto configs = json{};
	auto nextExpectedConfigurationIndex = ConfigurationIndex{ 0u };

	for (auto const& [configIndex, configTree] : configTrees)
	{
		// Start a new Context now, DescriptorIndexes start at 0 for each new configuration
		auto c = Context{};

		if (configIndex != nextExpectedConfigurationIndex)
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid Configuration Descriptor Index: " + std::to_string(configIndex) + " but expected " + std::to_string(nextExpectedConfigurationIndex) };
		}
		++nextExpectedConfigurationIndex;

		auto config = json{};

		// Dump Static model
		if (flags.test(SerializationFlag::SerializeStaticModel))
		{
			// Dump Configuration Descriptor Model
			config[keyName::Node_StaticInformation] = configTree.staticModel;
		}

		// Dump Dynamic model
		if (flags.test(SerializationFlag::SerializeDynamicModel))
		{
			// Dump Configuration Descriptor Model
			config[keyName::Node_DynamicInformation] = configTree.dynamicModel;
		}

		// Dump AudioUnits
		config[keyName::NodeName_AudioUnitDescriptors] = dumpAudioUnitModels(c, configTree, flags);

		// Dump StreamInputs
		config[keyName::NodeName_StreamInputDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::streamInputModels, c.nextExpectedStreamInputIndex, "StreamInput", 0, configTree.streamInputModels.size());

		// Dump StreamOutputs
		config[keyName::NodeName_StreamOutputDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::streamOutputModels, c.nextExpectedStreamOutputIndex, "StreamOutput", 0, configTree.streamOutputModels.size());

		// Dump AvbInterfaces
		config[keyName::NodeName_AvbInterfaceDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::avbInterfaceModels, c.nextExpectedAvbInterfaceIndex, "AvbInterface", 0, configTree.avbInterfaceModels.size());

		// Dump ClockSources
		config[keyName::NodeName_ClockSourceDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::clockSourceModels, c.nextExpectedClockSourceIndex, "ClockSource", 0, configTree.clockSourceModels.size());

		// Dump MemoryObjects
		config[keyName::NodeName_MemoryObjectDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::memoryObjectModels, c.nextExpectedMemoryObjectIndex, "MemoryObject", 0, configTree.memoryObjectModels.size());

		// Dump Locales
		config[keyName::NodeName_LocaleDescriptors] = dumpLocaleModels(c, configTree, flags);

		// Dump ClockDomains
		config[keyName::NodeName_ClockDomainDescriptors] = dumpLeafModels(configTree, flags, &ConfigurationTree::clockDomainModels, c.nextExpectedClockDomainIndex, "ClockDomain", 0, configTree.clockDomainModels.size());

		// Dump informative DescriptorIndex
		config[model::keyName::Node_Informative_Index] = configIndex;

		configs.push_back(std::move(config));
	}

	return configs;
}

nlohmann::json dumpEntityTree(EntityTree const& entityTree, SerializationFlags const flags)
{
	auto entity = json{};

	// Dump Static model
	if (flags.test(SerializationFlag::SerializeStaticModel))
	{
		entity[keyName::Node_StaticInformation] = entityTree.staticModel;
	}

	// Dump Dynamic model
	if (flags.test(SerializationFlag::SerializeDynamicModel))
	{
		entity[keyName::Node_DynamicInformation] = entityTree.dynamicModel;
	}

	// Dump Configurations
	entity[keyName::NodeName_ConfigurationDescriptors] = dumpConfigurationTrees(entityTree.configurationTrees, flags);

	return entity;
}

nlohmann::json LA_AVDECC_CALL_CONVENTION createJsonObject(EntityTree const& entityTree, SerializationFlags const flags)
{
	auto object = json{};

	object[keyName::NodeName_EntityDescriptor] = dumpEntityTree(entityTree, flags);

	return object;
}

} // namespace jsonSerializer
} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
