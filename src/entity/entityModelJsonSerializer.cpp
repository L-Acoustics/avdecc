/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
/* ************************************************************ */
/* Private structures                                           */
/* ************************************************************ */
struct Context
{
	AudioUnitIndex nextExpectedAudioUnitIndex{ 0u };
	StreamIndex nextExpectedStreamInputIndex{ 0u };
	StreamIndex nextExpectedStreamOutputIndex{ 0u };
	JackIndex nextExpectedJackInputIndex{ 0u };
	JackIndex nextExpectedJackOutputIndex{ 0u };
	AvbInterfaceIndex nextExpectedAvbInterfaceIndex{ 0u };
	ClockSourceIndex nextExpectedClockSourceIndex{ 0u };
	MemoryObjectIndex nextExpectedMemoryObjectIndex{ 0u };
	LocaleIndex nextExpectedLocaleIndex{ 0u };
	StringsIndex nextExpectedStringsIndex{ 0u };
	StreamPortIndex nextExpectedStreamPortInputIndex{ 0u };
	StreamPortIndex nextExpectedStreamPortOutputIndex{ 0u };
	ClusterIndex nextExpectedAudioClusterIndex{ 0u };
	MapIndex nextExpectedAudioMapIndex{ 0u };
	ControlIndex nextExpectedControlIndex{ 0u };
	ClockDomainIndex nextExpectedClockDomainIndex{ 0u };

	bool getSanityCheckError{ false };
};

/* ************************************************************ */
/* Dump methods                                                 */
/* ************************************************************ */
template<bool hasDynamicModel = true, typename FieldPointer>
json dumpLeafModels(Context& c, ConfigurationTree const& configTree, Flags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex& nextExpectedIndex, std::string const& descriptorName, DescriptorIndex const baseIndex, size_t const numberOfIndexes)
{
	auto objects = json{};

	for (auto descriptorIndexCounter = DescriptorIndex(0); descriptorIndexCounter < numberOfIndexes; ++descriptorIndexCounter)
	{
		auto const descriptorIndex = DescriptorIndex(descriptorIndexCounter + baseIndex);
		if (descriptorIndex != nextExpectedIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(descriptorIndex) + " but expected " + std::to_string(nextExpectedIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
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
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump Descriptor Model
			object[keyName::Node_StaticInformation] = models.staticModel;
		}

		// Dump Dynamic model
		if constexpr (hasDynamicModel)
		{
			if (flags.test(Flag::ProcessDynamicModel))
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
json dumpStringsModels(ConfigurationTree const& configTree, Flags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex const baseStrings, std::uint16_t const numberOfStrings)
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
		if (flags.test(Flag::ProcessStaticModel))
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
json dumpStreamPortModels(Context& c, ConfigurationTree const& configTree, Flags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex& nextExpectedIndex, DescriptorIndex const baseStreamPort, std::uint16_t const numberOfStreamPorts)
{
	auto streamPorts = json{};

	for (auto streamPortIndexCounter = StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
	{
		auto const streamPortIndex = StreamPortIndex(streamPortIndexCounter + baseStreamPort);
		if (streamPortIndex != nextExpectedIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid StreamPort Descriptor Index: " + std::to_string(streamPortIndex) + " but expected " + std::to_string(nextExpectedIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
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
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump StreamPort Descriptor Model
			streamPort[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump StreamPort Descriptor Model
			streamPort[keyName::Node_DynamicInformation] = streamPortModels.dynamicModel;
		}

		// Dump AudioClusters
		streamPort[keyName::NodeName_AudioClusterDescriptors] = dumpLeafModels(c, configTree, flags, &ConfigurationTree::audioClusterModels, c.nextExpectedAudioClusterIndex, "AudioCluster", staticModel.baseCluster, staticModel.numberOfClusters);

		// Dump AudioMaps
		streamPort[keyName::NodeName_AudioMapDescriptors] = dumpLeafModels<false>(c, configTree, flags, &ConfigurationTree::audioMapModels, c.nextExpectedAudioMapIndex, "AudioMap", staticModel.baseMap, staticModel.numberOfMaps);

		// Dump informative DescriptorIndex
		streamPort[model::keyName::Node_Informative_Index] = streamPortIndex;

		streamPorts.push_back(std::move(streamPort));
	}

	return streamPorts;
}

json dumpAudioUnitModels(Context& c, ConfigurationTree const& configTree, Flags const flags)
{
	auto audioUnits = json{};

	for (auto const& [audioUnitIndex, audioUnitModels] : configTree.audioUnitModels)
	{
		if (audioUnitIndex != c.nextExpectedAudioUnitIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid AudioUnit Descriptor Index: " + std::to_string(audioUnitIndex) + " but expected " + std::to_string(c.nextExpectedAudioUnitIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
		}
		++c.nextExpectedAudioUnitIndex;

		auto audioUnit = json{};

		// Dump Static model
		auto const& staticModel = audioUnitModels.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump AudioUnit Descriptor Model
			audioUnit[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump AudioUnit Descriptor Model
			audioUnit[keyName::Node_DynamicInformation] = audioUnitModels.dynamicModel;
		}

		// Dump StreamPortInputs
		audioUnit[keyName::NodeName_StreamPortInputDescriptors] = dumpStreamPortModels(c, configTree, flags, &ConfigurationTree::streamPortInputModels, c.nextExpectedStreamPortInputIndex, staticModel.baseStreamInputPort, staticModel.numberOfStreamInputPorts);

		// Dump StreamPortOutputs
		audioUnit[keyName::NodeName_StreamPortOutputDescriptors] = dumpStreamPortModels(c, configTree, flags, &ConfigurationTree::streamPortOutputModels, c.nextExpectedStreamPortOutputIndex, staticModel.baseStreamOutputPort, staticModel.numberOfStreamOutputPorts);

#pragma message("TODO: Dump other AUDIO_UNIT children")

		// Dump informative DescriptorIndex
		audioUnit[model::keyName::Node_Informative_Index] = audioUnitIndex;

		audioUnits.push_back(std::move(audioUnit));
	}

	return audioUnits;
}

json dumpLocaleModels(Context& c, ConfigurationTree const& configTree, Flags const flags)
{
	auto locales = json{};

	for (auto const& [localeIndex, localeModels] : configTree.localeModels)
	{
		if (localeIndex != c.nextExpectedLocaleIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid Locale Descriptor Index: " + std::to_string(localeIndex) + " but expected " + std::to_string(c.nextExpectedLocaleIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
		}
		++c.nextExpectedLocaleIndex;

		auto locale = json{};

		// Dump Static model
		auto const& staticModel = localeModels.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
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

json dumpConfigurationTrees(std::map<ConfigurationIndex, ConfigurationTree> const& configTrees, Flags const flags, bool& gotSanityCheckError)
{
	auto configs = json{};
	auto nextExpectedConfigurationIndex = ConfigurationIndex{ 0u };

	for (auto const& [configIndex, configTree] : configTrees)
	{
		// Start a new Context now, DescriptorIndexes start at 0 for each new configuration
		auto c = Context{};

		if (configIndex != nextExpectedConfigurationIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid Configuration Descriptor Index: " + std::to_string(configIndex) + " but expected " + std::to_string(nextExpectedConfigurationIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
		}
		++nextExpectedConfigurationIndex;

		auto config = json{};
		auto dumpFlags = flags;

		// Dump Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump Configuration Descriptor Model
			config[keyName::Node_StaticInformation] = configTree.staticModel;

#if 1
			// Until we are able to load VIDEO/SENSOR/CONTROL_BLOCK, we need to flag the device as incomplete because of possible CONTROLS at other levels of the model, breaking the numering
#	pragma message("TODO: Load VIDEO/SENSOR/CONTROL_BLOCK")
			if (configTree.staticModel.descriptorCounts.count(avdecc::entity::model::DescriptorType::VideoUnit) > 0 || configTree.staticModel.descriptorCounts.count(avdecc::entity::model::DescriptorType::SensorUnit) > 0 || configTree.staticModel.descriptorCounts.count(avdecc::entity::model::DescriptorType::ControlBlock) > 0)
			{
				if (!flags.test(Flag::IgnoreAEMSanityChecks))
				{
					throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::NotSupported, "Unsupported descriptor type: Video and/or Sensor and/or ControlBlock" };
				}
				else
				{
					c.getSanityCheckError = true;
				}
			}
#endif
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump Configuration Descriptor Model
			config[keyName::Node_DynamicInformation] = configTree.dynamicModel;
			// This is not the active configuration, we don't want to dump the Dynamic Part as it might not be accurate
			if (!configTree.dynamicModel.isActiveConfiguration)
			{
				dumpFlags.reset(Flag::ProcessDynamicModel);
			}
		}

		// Dump AudioUnits
		config[keyName::NodeName_AudioUnitDescriptors] = dumpAudioUnitModels(c, configTree, dumpFlags);

		// Dump StreamInputs
		config[keyName::NodeName_StreamInputDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::streamInputModels, c.nextExpectedStreamInputIndex, "StreamInput", 0, configTree.streamInputModels.size());

		// Dump StreamOutputs
		config[keyName::NodeName_StreamOutputDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::streamOutputModels, c.nextExpectedStreamOutputIndex, "StreamOutput", 0, configTree.streamOutputModels.size());

		// Dump JackInputs
		config[keyName::NodeName_JackInputDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::jackInputModels, c.nextExpectedJackInputIndex, "JackInput", 0, configTree.jackInputModels.size());

		// Dump JackOutputs
		config[keyName::NodeName_JackOutputDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::jackOutputModels, c.nextExpectedJackOutputIndex, "JackOutput", 0, configTree.jackOutputModels.size());

		// Dump AvbInterfaces
		config[keyName::NodeName_AvbInterfaceDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::avbInterfaceModels, c.nextExpectedAvbInterfaceIndex, "AvbInterface", 0, configTree.avbInterfaceModels.size());

		// Dump ClockSources
		config[keyName::NodeName_ClockSourceDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::clockSourceModels, c.nextExpectedClockSourceIndex, "ClockSource", 0, configTree.clockSourceModels.size());

		// Dump MemoryObjects
		config[keyName::NodeName_MemoryObjectDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::memoryObjectModels, c.nextExpectedMemoryObjectIndex, "MemoryObject", 0, configTree.memoryObjectModels.size());

		// Dump Locales
		config[keyName::NodeName_LocaleDescriptors] = dumpLocaleModels(c, configTree, dumpFlags);

		// Dump Controls
		config[keyName::NodeName_ControlDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::controlModels, c.nextExpectedControlIndex, "Control", 0, configTree.controlModels.size());

		// Dump ClockDomains
		config[keyName::NodeName_ClockDomainDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::clockDomainModels, c.nextExpectedClockDomainIndex, "ClockDomain", 0, configTree.clockDomainModels.size());

		// Dump informative DescriptorIndex
		config[model::keyName::Node_Informative_Index] = configIndex;

		configs.push_back(std::move(config));

		if (c.getSanityCheckError)
		{
			gotSanityCheckError = true;
		}
	}

	return configs;
}

json dumpEntityTree(EntityTree const& entityTree, Flags const flags, bool& gotSanityCheckError)
{
	auto entity = json{};

	// Dump Static model
	if (flags.test(Flag::ProcessStaticModel))
	{
		entity[keyName::Node_StaticInformation] = entityTree.staticModel;
	}

	// Dump Dynamic model
	if (flags.test(Flag::ProcessDynamicModel))
	{
		entity[keyName::Node_DynamicInformation] = entityTree.dynamicModel;
	}

	// Dump Configurations
	entity[keyName::NodeName_ConfigurationDescriptors] = dumpConfigurationTrees(entityTree.configurationTrees, flags, gotSanityCheckError);

	return entity;
}

json LA_AVDECC_CALL_CONVENTION createJsonObject(EntityTree const& entityTree, Flags const flags)
{
	try
	{
		auto object = json{};
		auto gotSanityCheckError = false;

		object[keyName::NodeName_EntityDescriptor] = dumpEntityTree(entityTree, flags, gotSanityCheckError);

		// If sanity checks failed
		if (gotSanityCheckError)
		{
			object[keyName::Node_NotCompliant] = true;
		}

		return object;
	}
	catch (json::exception const& e)
	{
		AVDECC_ASSERT(false, "json::exception is not expected to be thrown here");
		throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InternalError, e.what() };
	}
	catch (avdecc::jsonSerializer::SerializationException const&)
	{
		throw; // Rethrow, this is already the correct exception type
	}
	catch (...)
	{
		// Check that only SerializationException exception type propagate outside the shared library, otherwise it will be sliced and the caller won't be able to catch it properly (on macOS)
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::SerializationException should not propagate");
		throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InternalError, "Exception type other than avdecc::jsonSerializer::SerializationException should not propagate." };
	}
}

/* ************************************************************ */
/* Load methods                                                 */
/* ************************************************************ */
template<bool isKeyRequired = false, bool isStaticModelOptional = false, bool isDynamicModelOptional = false, bool hasDynamicModel = true, typename ModelTrees>
void readLeafModels(json const& object, Flags const flags, std::string const& keyName, DescriptorIndex& currentIndex, ModelTrees& modelTrees, [[maybe_unused]] bool const ignoreDynamicModel)
{
	auto const* obj = static_cast<json const*>(nullptr);

	if constexpr (isKeyRequired)
	{
		obj = &object.at(keyName);
	}
	else
	{
		auto const it = object.find(keyName);
		if (it == object.end())
		{
			return;
		}

		obj = &(*it);
	}

	for (auto const& j : *obj)
	{
		auto modelTree = typename ModelTrees::mapped_type{};

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			if constexpr (isStaticModelOptional)
			{
				get_optional_value(object, keyName::Node_StaticInformation, modelTree.staticModel);
			}
			else
			{
				j.at(keyName::Node_StaticInformation).get_to(modelTree.staticModel);
			}
		}

		// Read Dynamic model
		if constexpr (hasDynamicModel)
		{
			if (flags.test(Flag::ProcessDynamicModel) && !ignoreDynamicModel)
			{
				if constexpr (isDynamicModelOptional)
				{
					get_optional_value(j, keyName::Node_DynamicInformation, modelTree.dynamicModel);
				}
				else
				{
					j.at(keyName::Node_DynamicInformation).get_to(modelTree.dynamicModel);
				}
			}
		}

		modelTrees[currentIndex++] = std::move(modelTree);
	}
}

template<bool isKeyRequired = false, bool isStaticModelOptional = false, bool isDynamicModelOptional = false, bool hasDynamicModel = true, typename ModelTrees>
void readStreamPortModels(json const& object, Flags const flags, std::string const& keyName, DescriptorIndex& currentIndex, ModelTrees& modelTrees, Context& c, ConfigurationTree& config, bool const ignoreDynamicModel)
{
	auto const* obj = static_cast<json const*>(nullptr);

	if constexpr (isKeyRequired)
	{
		obj = &object.at(keyName);
	}
	else
	{
		auto const it = object.find(keyName);
		if (it == object.end())
		{
			return;
		}

		obj = &(*it);
	}

	for (auto const& j : *obj)
	{
		auto modelTree = typename ModelTrees::mapped_type{};

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get base cluster and map descriptor index
			modelTree.staticModel.baseCluster = c.nextExpectedAudioClusterIndex;
			modelTree.staticModel.baseMap = c.nextExpectedAudioMapIndex;

			if constexpr (isStaticModelOptional)
			{
				get_optional_value(object, keyName::Node_StaticInformation, modelTree.staticModel);
			}
			else
			{
				j.at(keyName::Node_StaticInformation).get_to(modelTree.staticModel);
			}
		}

		// Read Dynamic model
		if constexpr (hasDynamicModel)
		{
			if (flags.test(Flag::ProcessDynamicModel) && !ignoreDynamicModel)
			{
				if constexpr (isDynamicModelOptional)
				{
					get_optional_value(j, keyName::Node_DynamicInformation, modelTree.dynamicModel);
				}
				else
				{
					j.at(keyName::Node_DynamicInformation).get_to(modelTree.dynamicModel);
				}
			}
		}

		// Read AudioClusters
		readLeafModels<true, false, true>(j, flags, keyName::NodeName_AudioClusterDescriptors, c.nextExpectedAudioClusterIndex, config.audioClusterModels, ignoreDynamicModel);

		// Read AudioMaps
		readLeafModels<false, false, true, false>(j, flags, keyName::NodeName_AudioMapDescriptors, c.nextExpectedAudioMapIndex, config.audioMapModels, ignoreDynamicModel);

		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get number of cluster and map descriptors that were read
			modelTree.staticModel.numberOfClusters = c.nextExpectedAudioClusterIndex - modelTree.staticModel.baseCluster;
			modelTree.staticModel.numberOfMaps = c.nextExpectedAudioMapIndex - modelTree.staticModel.baseMap;
			modelTree.staticModel.hasDynamicAudioMap = modelTree.staticModel.numberOfMaps == 0;
		}

		modelTrees[currentIndex++] = std::move(modelTree);
	}
}

void readAudioUnitModels(json const& object, Flags const flags, Context& c, ConfigurationTree& config, bool const ignoreDynamicModel)
{
	for (auto const& j : object)
	{
		auto audioUnitTree = AudioUnitNodeModels{};

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get base stream port descriptor index
			audioUnitTree.staticModel.baseStreamInputPort = c.nextExpectedStreamPortInputIndex;
			audioUnitTree.staticModel.baseStreamOutputPort = c.nextExpectedStreamPortOutputIndex;

			j.at(keyName::Node_StaticInformation).get_to(audioUnitTree.staticModel);
		}

		// Read Dynamic model
		if (flags.test(Flag::ProcessDynamicModel) && !ignoreDynamicModel)
		{
			j.at(keyName::Node_DynamicInformation).get_to(audioUnitTree.dynamicModel);
		}

		// Read StreamPortInputs
		readStreamPortModels<false, false, true>(j, flags, keyName::NodeName_StreamPortInputDescriptors, c.nextExpectedStreamPortInputIndex, config.streamPortInputModels, c, config, ignoreDynamicModel);

		// Read StreamPortOutputs
		readStreamPortModels<false, false, true>(j, flags, keyName::NodeName_StreamPortOutputDescriptors, c.nextExpectedStreamPortOutputIndex, config.streamPortOutputModels, c, config, ignoreDynamicModel);

#pragma message("TODO: Read other AUDIO_UNIT children")

		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get number of stream port descriptors that were read
			audioUnitTree.staticModel.numberOfStreamInputPorts = c.nextExpectedStreamPortInputIndex - audioUnitTree.staticModel.baseStreamInputPort;
			audioUnitTree.staticModel.numberOfStreamOutputPorts = c.nextExpectedStreamPortOutputIndex - audioUnitTree.staticModel.baseStreamOutputPort;
		}

		config.audioUnitModels[c.nextExpectedAudioUnitIndex++] = std::move(audioUnitTree);
	}
}

void readLocaleModels(json const& object, Flags const flags, Context& c, ConfigurationTree& config, bool const ignoreDynamicModel)
{
	for (auto const& j : object)
	{
		auto localeTree = LocaleNodeModels{};

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			j.at(keyName::Node_StaticInformation).get_to(localeTree.staticModel);

			// Get base strings descriptor index
			localeTree.staticModel.baseStringDescriptorIndex = c.nextExpectedStringsIndex;

			// Read Strings
			readLeafModels<true, false, true, false>(j, flags, keyName::NodeName_StringsDescriptors, c.nextExpectedStringsIndex, config.stringsModels, ignoreDynamicModel);

			// Get number of strings descriptors that were read
			localeTree.staticModel.numberOfStringDescriptors = c.nextExpectedStringsIndex - localeTree.staticModel.baseStringDescriptorIndex;
		}

		config.localeModels[c.nextExpectedLocaleIndex++] = std::move(localeTree);
	}
}

EntityTree::ConfigurationTrees readConfigurationTrees(json const& object, Flags const flags, std::optional<DescriptorIndex> const currentConfiguration)
{
	auto configurationTrees = EntityTree::ConfigurationTrees{};
	auto configurationIndex = ConfigurationIndex{ 0u };

	for (auto const& j : object)
	{
		// Start a new Context now, DescriptorIndexes start at 0 for each new configuration
		auto c = Context{};

		auto config = ConfigurationTree{};

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			get_optional_value(j, keyName::Node_StaticInformation, config.staticModel);
		}

		// Read Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			get_optional_value(j, keyName::Node_DynamicInformation, config.dynamicModel);
			// Set active configuration
			if (currentConfiguration && (*currentConfiguration == configurationIndex))
			{
				config.dynamicModel.isActiveConfiguration = true;
			}
		}

		auto const ignoreDynamicModel = currentConfiguration ? (*currentConfiguration != configurationIndex) : false;

		// Read AudioUnits
		if (auto const jtree = j.find(keyName::NodeName_AudioUnitDescriptors); jtree != j.end())
		{
			readAudioUnitModels(*jtree, flags, c, config, ignoreDynamicModel);
		}

		// Read StreamInputs
		readLeafModels(j, flags, keyName::NodeName_StreamInputDescriptors, c.nextExpectedStreamInputIndex, config.streamInputModels, ignoreDynamicModel);

		// Read StreamOutputs
		readLeafModels(j, flags, keyName::NodeName_StreamOutputDescriptors, c.nextExpectedStreamOutputIndex, config.streamOutputModels, ignoreDynamicModel);

		// Read JackInputs
		readLeafModels(j, flags, keyName::NodeName_JackInputDescriptors, c.nextExpectedJackInputIndex, config.jackInputModels, ignoreDynamicModel);

		// Read JackOutputs
		readLeafModels(j, flags, keyName::NodeName_JackOutputDescriptors, c.nextExpectedJackOutputIndex, config.jackOutputModels, ignoreDynamicModel);

		// Read AvbInterfaces
		readLeafModels<false, false, true>(j, flags, keyName::NodeName_AvbInterfaceDescriptors, c.nextExpectedAvbInterfaceIndex, config.avbInterfaceModels, ignoreDynamicModel);

		// Read ClockSources
		readLeafModels<false, false, true>(j, flags, keyName::NodeName_ClockSourceDescriptors, c.nextExpectedClockSourceIndex, config.clockSourceModels, ignoreDynamicModel);

		// Read MemoryObjects
		readLeafModels(j, flags, keyName::NodeName_MemoryObjectDescriptors, c.nextExpectedMemoryObjectIndex, config.memoryObjectModels, ignoreDynamicModel);

		// Read Locales
		if (auto const jtree = j.find(keyName::NodeName_LocaleDescriptors); jtree != j.end())
		{
			readLocaleModels(*jtree, flags, c, config, ignoreDynamicModel);
		}

		// Read Controls
		readLeafModels(j, flags, keyName::NodeName_ControlDescriptors, c.nextExpectedControlIndex, config.controlModels, ignoreDynamicModel);

		// Read ClockDomains
		readLeafModels(j, flags, keyName::NodeName_ClockDomainDescriptors, c.nextExpectedClockDomainIndex, config.clockDomainModels, ignoreDynamicModel);

		configurationTrees[configurationIndex++] = std::move(config);
	}

	return configurationTrees;
}

EntityTree readEntityTree(json const& object, Flags const flags)
{
	auto entityTree = EntityTree{};
	auto currentConfiguration = std::optional<DescriptorIndex>{ std::nullopt };

	// Read Static model
	if (flags.test(Flag::ProcessStaticModel))
	{
		get_optional_value(object, keyName::Node_StaticInformation, entityTree.staticModel);
	}

	// Read Dynamic model
	if (flags.test(Flag::ProcessDynamicModel))
	{
		object.at(keyName::Node_DynamicInformation).get_to(entityTree.dynamicModel);
		currentConfiguration = entityTree.dynamicModel.currentConfiguration;
	}

	// Read Configurations
	auto const configs = object.find(keyName::NodeName_ConfigurationDescriptors);
	if (configs != object.end())
	{
		entityTree.configurationTrees = readConfigurationTrees(*configs, flags, currentConfiguration);
	}

	return entityTree;
}

EntityTree LA_AVDECC_CALL_CONVENTION createEntityTree(json const& object, Flags const flags)
{
	try
	{
		// Check for compliance
		{
			auto notCompliant = false;
			get_optional_value(object, keyName::Node_NotCompliant, notCompliant);
			if (notCompliant && !flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::NotCompliant, "Model is not fully compliant with IEEE1722.1, or is incomplete." };
			}
		}

		return readEntityTree(object.at(keyName::NodeName_EntityDescriptor), flags);
	}
	catch (json::type_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what() };
	}
	catch (json::parse_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::ParseError, e.what() };
	}
	catch (json::out_of_range const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::MissingKey, e.what() };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what() };
		}
		else
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
		}
	}
	catch (json::exception const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
	}
	catch (avdecc::jsonSerializer::DeserializationException const&)
	{
		throw; // Rethrow, this is already the correct exception type
	}
	catch (std::invalid_argument const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what() };
	}
	catch (...)
	{
		// Check that only DeserializationException exception type propagate outside the shared library, otherwise it will be sliced and the caller won't be able to catch it properly (on macOS)
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::DeserializationException should not propagate.");
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, "Exception type other than avdecc::jsonSerializer::DeserializationException should not propagate." };
	}
}

} // namespace jsonSerializer
} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
