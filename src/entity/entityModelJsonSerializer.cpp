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
	TimingIndex nextExpectedTimingIndex{ 0u };
	PtpInstanceIndex nextExpectedPtpInstanceIndex{ 0u };
	PtpPortIndex nextExpectedPtpPortIndex{ 0u };

	bool getSanityCheckError{ false };
};

/* ************************************************************ */
/* Dump methods                                                 */
/* ************************************************************ */
template<bool hasDynamicModel = true, class ParentTree, typename FieldPointer>
json dumpLeafModels(Context& c, ParentTree const& parentTree, Flags const flags, FieldPointer ParentTree::*const Field, DescriptorIndex& nextExpectedIndex, std::string const& descriptorName, DescriptorIndex const baseIndex, size_t const numberOfIndexes)
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

		auto const modelsIt = (parentTree.*Field).find(descriptorIndex);
		if (modelsIt == (parentTree.*Field).end())
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
json dumpStringsModels(LocaleTree const& localeTree, Flags const flags, FieldPointer LocaleTree::*const Field, DescriptorIndex const baseStrings, std::uint16_t const numberOfStrings)
{
	auto strings = json{};

	for (auto stringsIndexCounter = StringsIndex(0); stringsIndexCounter < numberOfStrings; ++stringsIndexCounter)
	{
		auto const stringsIndex = StringsIndex(stringsIndexCounter + baseStrings);
		auto const stringsModelsIt = (localeTree.*Field).find(stringsIndex);
		if (stringsModelsIt == (localeTree.*Field).end())
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
json dumpStreamPortModels(Context& c, AudioUnitTree const& audioUnitTree, Flags const flags, FieldPointer AudioUnitTree::*const Field, DescriptorIndex& nextExpectedIndex, std::string const& descriptorName, DescriptorIndex const baseStreamPort, std::uint16_t const numberOfStreamPorts)
{
	auto streamPorts = json{};

	for (auto streamPortIndexCounter = StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
	{
		auto const streamPortIndex = StreamPortIndex(streamPortIndexCounter + baseStreamPort);
		if (streamPortIndex != nextExpectedIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(streamPortIndex) + " but expected " + std::to_string(nextExpectedIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
		}
		++nextExpectedIndex;

		auto const streamPortTreeIt = (audioUnitTree.*Field).find(streamPortIndex);
		if (streamPortTreeIt == (audioUnitTree.*Field).end())
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(streamPortIndex) + " (out of range)" };
		}
		auto const& streamPortTree = streamPortTreeIt->second;

		auto streamPort = json{};

		// Dump Static model
		auto const& staticModel = streamPortTree.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump StreamPort Descriptor Model
			streamPort[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump StreamPort Descriptor Model
			streamPort[keyName::Node_DynamicInformation] = streamPortTree.dynamicModel;
		}

		// Dump AudioClusters
		streamPort[keyName::NodeName_AudioClusterDescriptors] = dumpLeafModels(c, streamPortTree, flags, &StreamPortTree::audioClusterModels, c.nextExpectedAudioClusterIndex, "AudioCluster", staticModel.baseCluster, staticModel.numberOfClusters);

		// Dump AudioMaps
		streamPort[keyName::NodeName_AudioMapDescriptors] = dumpLeafModels<false>(c, streamPortTree, flags, &StreamPortTree::audioMapModels, c.nextExpectedAudioMapIndex, "AudioMap", staticModel.baseMap, staticModel.numberOfMaps);

		// Dump Controls
		streamPort[keyName::NodeName_ControlDescriptors] = dumpLeafModels(c, streamPortTree, flags, &StreamPortTree::controlModels, c.nextExpectedControlIndex, "Control", staticModel.baseControl, staticModel.numberOfControls);

		// Dump informative DescriptorIndex
		streamPort[model::keyName::Node_Informative_Index] = streamPortIndex;

		streamPorts.push_back(std::move(streamPort));
	}

	return streamPorts;
}

json dumpAudioUnitModels(Context& c, ConfigurationTree const& configTree, Flags const flags)
{
	auto audioUnits = json{};

	for (auto const& [audioUnitIndex, audioUnitTree] : configTree.audioUnitTrees)
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
		auto const& staticModel = audioUnitTree.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump AudioUnit Descriptor Model
			audioUnit[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump AudioUnit Descriptor Model
			audioUnit[keyName::Node_DynamicInformation] = audioUnitTree.dynamicModel;
		}

		// We first need to dump leaves, as some trees may contain the same type of leaves we can find at the configuration level (eg. Controls)
		{
			// Dump Controls
			audioUnit[keyName::NodeName_ControlDescriptors] = dumpLeafModels(c, audioUnitTree, flags, &AudioUnitTree::controlModels, c.nextExpectedControlIndex, "Control", staticModel.baseControl, staticModel.numberOfControls);
		}

		// Now we can dump the trees
		{
			// Dump StreamPortInputs
			audioUnit[keyName::NodeName_StreamPortInputDescriptors] = dumpStreamPortModels(c, audioUnitTree, flags, &AudioUnitTree::streamPortInputTrees, c.nextExpectedStreamPortInputIndex, "StreamPortInput", staticModel.baseStreamInputPort, staticModel.numberOfStreamInputPorts);

			// Dump StreamPortOutputs
			audioUnit[keyName::NodeName_StreamPortOutputDescriptors] = dumpStreamPortModels(c, audioUnitTree, flags, &AudioUnitTree::streamPortOutputTrees, c.nextExpectedStreamPortOutputIndex, "StreamPortOutput", staticModel.baseStreamOutputPort, staticModel.numberOfStreamOutputPorts);
		}

		// Dump informative DescriptorIndex
		audioUnit[model::keyName::Node_Informative_Index] = audioUnitIndex;

		audioUnits.push_back(std::move(audioUnit));
	}

	return audioUnits;
}

template<typename FieldPointer>
json dumpJackModels(Context& c, ConfigurationTree const& configTree, Flags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex& nextExpectedIndex, std::string const& descriptorName, DescriptorIndex const baseJack, std::uint16_t const numberOfJacks)
{
	auto jacks = json{};

	for (auto jackIndexCounter = JackIndex(0); jackIndexCounter < numberOfJacks; ++jackIndexCounter)
	{
		auto const jackIndex = JackIndex(jackIndexCounter + baseJack);
		if (jackIndex != nextExpectedIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(jackIndex) + " but expected " + std::to_string(nextExpectedIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
		}
		++nextExpectedIndex;

		auto const jackTreeIt = (configTree.*Field).find(jackIndex);
		if (jackTreeIt == (configTree.*Field).end())
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(jackIndex) + " (out of range)" };
		}
		auto const& jackTree = jackTreeIt->second;

		auto jack = json{};

		// Dump Static model
		auto const& staticModel = jackTree.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump Jack Descriptor Model
			jack[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump Jack Descriptor Model
			jack[keyName::Node_DynamicInformation] = jackTree.dynamicModel;
		}

		// Dump Controls
		jack[keyName::NodeName_ControlDescriptors] = dumpLeafModels(c, jackTree, flags, &JackTree::controlModels, c.nextExpectedControlIndex, "Control", staticModel.baseControl, staticModel.numberOfControls);

		// Dump informative DescriptorIndex
		jack[model::keyName::Node_Informative_Index] = jackIndex;

		jacks.push_back(std::move(jack));
	}

	return jacks;
}

template<typename FieldPointer>
json dumpPtpInstanceModels(Context& c, ConfigurationTree const& configTree, Flags const flags, FieldPointer ConfigurationTree::*const Field, DescriptorIndex& nextExpectedIndex, std::string const& descriptorName, DescriptorIndex const basePtpInstance, std::uint16_t const numberOfPtpInstances)
{
	auto ptpInstances = json{};

	for (auto ptpInstanceIndexCounter = PtpInstanceIndex(0); ptpInstanceIndexCounter < numberOfPtpInstances; ++ptpInstanceIndexCounter)
	{
		auto const ptpInstanceIndex = PtpInstanceIndex(ptpInstanceIndexCounter + basePtpInstance);
		if (ptpInstanceIndex != nextExpectedIndex)
		{
			if (!flags.test(Flag::IgnoreAEMSanityChecks))
			{
				throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(ptpInstanceIndex) + " but expected " + std::to_string(nextExpectedIndex) };
			}
			else
			{
				c.getSanityCheckError = true;
			}
		}
		++nextExpectedIndex;

		auto const ptpInstanceTreeIt = (configTree.*Field).find(ptpInstanceIndex);
		if (ptpInstanceTreeIt == (configTree.*Field).end())
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex, "Invalid " + descriptorName + " Descriptor Index: " + std::to_string(ptpInstanceIndex) + " (out of range)" };
		}
		auto const& ptpInstanceTree = ptpInstanceTreeIt->second;

		auto ptpInstance = json{};

		// Dump Static model
		auto const& staticModel = ptpInstanceTree.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump PtpInstance Descriptor Model
			ptpInstance[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Dynamic model
		if (flags.test(Flag::ProcessDynamicModel))
		{
			// Dump PtpInstance Descriptor Model
			ptpInstance[keyName::Node_DynamicInformation] = ptpInstanceTree.dynamicModel;
		}

		// Dump Controls
		ptpInstance[keyName::NodeName_ControlDescriptors] = dumpLeafModels(c, ptpInstanceTree, flags, &PtpInstanceTree::controlModels, c.nextExpectedControlIndex, "Control", staticModel.baseControl, staticModel.numberOfControls);

		// Dump PtpPorts
		ptpInstance[keyName::NodeName_PtpPortDescriptors] = dumpLeafModels(c, ptpInstanceTree, flags, &PtpInstanceTree::ptpPortModels, c.nextExpectedPtpPortIndex, "PtpPort", staticModel.basePtpPort, staticModel.numberOfPtpPorts);

		// Dump informative DescriptorIndex
		ptpInstance[model::keyName::Node_Informative_Index] = ptpInstanceIndex;

		ptpInstances.push_back(std::move(ptpInstance));
	}

	return ptpInstances;
}

json dumpLocaleModels(Context& c, ConfigurationTree const& configTree, Flags const flags)
{
	auto locales = json{};

	for (auto const& [localeIndex, localeTree] : configTree.localeTrees)
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
		auto const& staticModel = localeTree.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump Locale Descriptor Model
			locale[keyName::Node_StaticInformation] = staticModel;
		}

		// Dump Strings
		locale[keyName::NodeName_StringsDescriptors] = dumpStringsModels(localeTree, flags, &LocaleTree::stringsModels, staticModel.baseStringDescriptorIndex, staticModel.numberOfStringDescriptors);

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
		auto const& staticModel = configTree.staticModel;
		if (flags.test(Flag::ProcessStaticModel))
		{
			// Dump Configuration Descriptor Model
			config[keyName::Node_StaticInformation] = staticModel;

#if 1
			// Until we are able to load VIDEO/SENSOR/CONTROL_BLOCK, we need to flag the device as incomplete because of possible CONTROLS at other levels of the model, breaking the numbering
#	pragma message("TODO: Load VIDEO/SENSOR/CONTROL_BLOCK")
			if (staticModel.descriptorCounts.count(avdecc::entity::model::DescriptorType::VideoUnit) > 0 || staticModel.descriptorCounts.count(avdecc::entity::model::DescriptorType::SensorUnit) > 0 || staticModel.descriptorCounts.count(avdecc::entity::model::DescriptorType::ControlBlock) > 0)
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

		// We first need to dump leaves, as some trees may contain the same type of leaves we can find at the configuration level (eg. Controls)
		{
			// Dump StreamInputs
			config[keyName::NodeName_StreamInputDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::streamInputModels, c.nextExpectedStreamInputIndex, "StreamInput", 0, configTree.streamInputModels.size());

			// Dump StreamOutputs
			config[keyName::NodeName_StreamOutputDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::streamOutputModels, c.nextExpectedStreamOutputIndex, "StreamOutput", 0, configTree.streamOutputModels.size());

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

			// Dump Timings
			config[keyName::NodeName_TimingDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::timingModels, c.nextExpectedTimingIndex, "Timing", 0, configTree.timingModels.size());
		}

		// Now we can dump the trees
		{
			// Dump AudioUnits
			config[keyName::NodeName_AudioUnitDescriptors] = dumpAudioUnitModels(c, configTree, dumpFlags);

			// Dump JackInputs
			config[keyName::NodeName_JackInputDescriptors] = dumpJackModels(c, configTree, dumpFlags, &ConfigurationTree::jackInputTrees, c.nextExpectedJackInputIndex, "JackInput", 0, static_cast<std::uint16_t>(configTree.jackInputTrees.size()));

			// Dump JackOutputs
			config[keyName::NodeName_JackOutputDescriptors] = dumpJackModels(c, configTree, dumpFlags, &ConfigurationTree::jackOutputTrees, c.nextExpectedJackOutputIndex, "JackOutput", 0, static_cast<std::uint16_t>(configTree.jackOutputTrees.size()));

			// Dump AvbInterfaces
			// Will be a tree in 1722.1-2021
			config[keyName::NodeName_AvbInterfaceDescriptors] = dumpLeafModels(c, configTree, dumpFlags, &ConfigurationTree::avbInterfaceModels, c.nextExpectedAvbInterfaceIndex, "AvbInterface", 0, configTree.avbInterfaceModels.size());

			// Dump PtpInstances
			config[keyName::NodeName_PtpInstanceDescriptors] = dumpPtpInstanceModels(c, configTree, dumpFlags, &ConfigurationTree::ptpInstanceTrees, c.nextExpectedPtpInstanceIndex, "PtpInstance", 0, static_cast<std::uint16_t>(configTree.ptpInstanceTrees.size()));
		}

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

					// Special handling of CONTROL descriptors that were missing the 'number_of_values' field (in the static model) in previous dump versions
					if constexpr (std::is_same_v<typename ModelTrees::mapped_type, ControlNodeModels>)
					{
						if (flags.test(Flag::ProcessStaticModel) && modelTree.staticModel.numberOfValues == 0)
						{
							modelTree.staticModel.numberOfValues = static_cast<std::uint16_t>(modelTree.dynamicModel.values.size());
						}
					}
				}
			}
		}

		modelTrees[currentIndex++] = std::move(modelTree);
	}
}

template<bool isKeyRequired = false, bool isStaticModelOptional = false, bool isDynamicModelOptional = false, bool hasDynamicModel = true, typename ModelTrees>
void readStreamPortModels(json const& object, Flags const flags, std::string const& keyName, DescriptorIndex& currentIndex, ModelTrees& modelTrees, Context& c, bool const ignoreDynamicModel)
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
		auto const baseCluster = c.nextExpectedAudioClusterIndex;
		auto const baseMap = c.nextExpectedAudioMapIndex;
		auto const baseControl = c.nextExpectedControlIndex;

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

		// Read AudioClusters
		readLeafModels<true, false, true>(j, flags, keyName::NodeName_AudioClusterDescriptors, c.nextExpectedAudioClusterIndex, modelTree.audioClusterModels, ignoreDynamicModel);

		// Read AudioMaps
		readLeafModels<false, false, true, false>(j, flags, keyName::NodeName_AudioMapDescriptors, c.nextExpectedAudioMapIndex, modelTree.audioMapModels, ignoreDynamicModel);

		// Read Controls
		readLeafModels(j, flags, keyName::NodeName_ControlDescriptors, c.nextExpectedControlIndex, modelTree.controlModels, ignoreDynamicModel);

		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get number of descriptors that were read
			auto const numberOfClusters = static_cast<decltype(baseCluster)>(c.nextExpectedAudioClusterIndex - baseCluster);
			auto const numberOfMaps = static_cast<decltype(baseMap)>(c.nextExpectedAudioMapIndex - baseMap);
			auto const numberOfControls = static_cast<decltype(baseControl)>(c.nextExpectedControlIndex - baseControl);
			// Only update fields if at least one descriptor was read
			if (numberOfClusters > 0)
			{
				modelTree.staticModel.baseCluster = baseCluster;
				modelTree.staticModel.numberOfClusters = numberOfClusters;
			}
			if (numberOfMaps > 0)
			{
				modelTree.staticModel.baseMap = baseMap;
				modelTree.staticModel.numberOfMaps = numberOfMaps;
			}
			if (numberOfControls > 0)
			{
				modelTree.staticModel.baseControl = baseControl;
				modelTree.staticModel.numberOfControls = numberOfControls;
			}
			modelTree.staticModel.hasDynamicAudioMap = modelTree.staticModel.numberOfMaps == 0;
		}

		modelTrees[currentIndex++] = std::move(modelTree);
	}
}

void readAudioUnitModels(json const& object, Flags const flags, Context& c, ConfigurationTree& config, bool const ignoreDynamicModel)
{
	for (auto const& j : object)
	{
		auto audioUnitTree = AudioUnitTree{};
		auto const baseStreamInputPort = c.nextExpectedStreamPortInputIndex;
		auto const baseStreamOutputPort = c.nextExpectedStreamPortOutputIndex;
		auto const baseControl = c.nextExpectedControlIndex;

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			j.at(keyName::Node_StaticInformation).get_to(audioUnitTree.staticModel);
		}

		// Read Dynamic model
		if (flags.test(Flag::ProcessDynamicModel) && !ignoreDynamicModel)
		{
			j.at(keyName::Node_DynamicInformation).get_to(audioUnitTree.dynamicModel);
		}

		// We first need to read leaves, as some trees may contain the same type of leaves we can find at the configuration level (eg. Controls)
		{
			// Read Controls
			readLeafModels(j, flags, keyName::NodeName_ControlDescriptors, c.nextExpectedControlIndex, audioUnitTree.controlModels, ignoreDynamicModel);
		}

		// Now we can read the trees
		{
			// Read StreamPortInputs
			readStreamPortModels<false, false, true>(j, flags, keyName::NodeName_StreamPortInputDescriptors, c.nextExpectedStreamPortInputIndex, audioUnitTree.streamPortInputTrees, c, ignoreDynamicModel);

			// Read StreamPortOutputs
			readStreamPortModels<false, false, true>(j, flags, keyName::NodeName_StreamPortOutputDescriptors, c.nextExpectedStreamPortOutputIndex, audioUnitTree.streamPortOutputTrees, c, ignoreDynamicModel);
		}

		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get number of descriptors that were read
			auto const numberOfStreamInputPorts = static_cast<decltype(baseStreamInputPort)>(c.nextExpectedStreamPortInputIndex - baseStreamInputPort);
			auto const numberOfStreamOutputPorts = static_cast<decltype(baseStreamOutputPort)>(c.nextExpectedStreamPortOutputIndex - baseStreamOutputPort);
			auto const numberOfControls = static_cast<decltype(baseControl)>(c.nextExpectedControlIndex - baseControl);
			// Only update fields if at least one descriptor was read
			if (numberOfStreamInputPorts > 0)
			{
				audioUnitTree.staticModel.baseStreamInputPort = baseStreamInputPort;
				audioUnitTree.staticModel.numberOfStreamInputPorts = numberOfStreamInputPorts;
			}
			if (numberOfStreamOutputPorts > 0)
			{
				audioUnitTree.staticModel.baseStreamOutputPort = baseStreamOutputPort;
				audioUnitTree.staticModel.numberOfStreamOutputPorts = numberOfStreamOutputPorts;
			}
			if (numberOfControls > 0)
			{
				audioUnitTree.staticModel.baseControl = baseControl;
				audioUnitTree.staticModel.numberOfControls = numberOfControls;
			}
		}

		config.audioUnitTrees[c.nextExpectedAudioUnitIndex++] = std::move(audioUnitTree);
	}
}

template<bool isKeyRequired = false, bool isStaticModelOptional = false, bool isDynamicModelOptional = false, bool hasDynamicModel = true, typename ModelTrees>
void readJackModels(json const& object, Flags const flags, std::string const& keyName, DescriptorIndex& currentIndex, ModelTrees& modelTrees, Context& c, bool const ignoreDynamicModel)
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
		auto const baseControl = c.nextExpectedControlIndex;

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

		// Read Controls
		readLeafModels(j, flags, keyName::NodeName_ControlDescriptors, c.nextExpectedControlIndex, modelTree.controlModels, ignoreDynamicModel);

		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get number of descriptors that were read
			auto const numberOfControls = static_cast<decltype(baseControl)>(c.nextExpectedControlIndex - baseControl);
			// Only update fields if at least one descriptor was read
			if (numberOfControls > 0)
			{
				modelTree.staticModel.baseControl = baseControl;
				modelTree.staticModel.numberOfControls = numberOfControls;
			}
		}

		modelTrees[currentIndex++] = std::move(modelTree);
	}
}

template<bool isKeyRequired = false, bool isStaticModelOptional = false, bool isDynamicModelOptional = false, bool hasDynamicModel = true, typename ModelTrees>
void readPtpInstanceModels(json const& object, Flags const flags, std::string const& keyName, DescriptorIndex& currentIndex, ModelTrees& modelTrees, Context& c, bool const ignoreDynamicModel)
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
		auto const baseControl = c.nextExpectedControlIndex;

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

		// Read Controls
		readLeafModels(j, flags, keyName::NodeName_ControlDescriptors, c.nextExpectedControlIndex, modelTree.controlModels, ignoreDynamicModel);

		// Read PtpPorts
		readLeafModels(j, flags, keyName::NodeName_PtpPortDescriptors, c.nextExpectedPtpPortIndex, modelTree.ptpPortModels, ignoreDynamicModel);

		if (flags.test(Flag::ProcessStaticModel))
		{
			// Get number of descriptors that were read
			auto const numberOfControls = static_cast<decltype(baseControl)>(c.nextExpectedControlIndex - baseControl);
			// Only update fields if at least one descriptor was read
			if (numberOfControls > 0)
			{
				modelTree.staticModel.baseControl = baseControl;
				modelTree.staticModel.numberOfControls = numberOfControls;
			}
			modelTree.staticModel.numberOfPtpPorts = c.nextExpectedPtpPortIndex - modelTree.staticModel.basePtpPort;
		}

		modelTrees[currentIndex++] = std::move(modelTree);
	}
}

void readLocaleModels(json const& object, Flags const flags, Context& c, ConfigurationTree& config, bool const ignoreDynamicModel)
{
	for (auto const& j : object)
	{
		auto localeTree = LocaleTree{};

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			j.at(keyName::Node_StaticInformation).get_to(localeTree.staticModel);

			// Get base strings descriptor index
			localeTree.staticModel.baseStringDescriptorIndex = c.nextExpectedStringsIndex;

			// Read Strings
			readLeafModels<true, false, true, false>(j, flags, keyName::NodeName_StringsDescriptors, c.nextExpectedStringsIndex, localeTree.stringsModels, ignoreDynamicModel);

			// Get number of strings descriptors that were read
			localeTree.staticModel.numberOfStringDescriptors = c.nextExpectedStringsIndex - localeTree.staticModel.baseStringDescriptorIndex;
		}

		config.localeTrees[c.nextExpectedLocaleIndex++] = std::move(localeTree);
	}
}

template<typename ModelTrees>
void setDescriptorCount(DescriptorCounts& counts, DescriptorType const descriptorType, ModelTrees const& modelTrees)
{
	if (!modelTrees.empty())
	{
		counts[descriptorType] = static_cast<DescriptorCounts::mapped_type>(modelTrees.size());
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
		auto mustRebuildDescriptorCount = false;

		// Read Static model
		if (flags.test(Flag::ProcessStaticModel))
		{
			get_optional_value(j, keyName::Node_StaticInformation, config.staticModel);
			// Check for old dump file
			if (config.staticModel.descriptorCounts.empty())
			{
				// Make sure the static model key exists
				if (auto const confStaticModelIt = j.find(keyName::Node_StaticInformation); confStaticModelIt != j.end())
				{
					// If the descriptor count is missing (ie. old dump file), we need to rebuild it
					if (confStaticModelIt->find(keyName::ConfigurationNode_Static_DescriptorCounts) == confStaticModelIt->end())
					{
						mustRebuildDescriptorCount = true;
					}
				}
			}
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

		// We first need to read leaves, as some trees may contain the same type of leaves we can find at the configuration level (eg. Controls)
		{
			// Read StreamInputs
			readLeafModels(j, flags, keyName::NodeName_StreamInputDescriptors, c.nextExpectedStreamInputIndex, config.streamInputModels, ignoreDynamicModel);

			// Read StreamOutputs
			readLeafModels(j, flags, keyName::NodeName_StreamOutputDescriptors, c.nextExpectedStreamOutputIndex, config.streamOutputModels, ignoreDynamicModel);

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

			// Read Timings
			readLeafModels(j, flags, keyName::NodeName_TimingDescriptors, c.nextExpectedTimingIndex, config.timingModels, ignoreDynamicModel);
		}

		// Now we can read the trees
		{
			// Read AudioUnits
			if (auto const jtree = j.find(keyName::NodeName_AudioUnitDescriptors); jtree != j.end())
			{
				readAudioUnitModels(*jtree, flags, c, config, ignoreDynamicModel);
			}

			// Read JackInputs
			readJackModels(j, flags, keyName::NodeName_JackInputDescriptors, c.nextExpectedJackInputIndex, config.jackInputTrees, c, ignoreDynamicModel);

			// Read JackOutputs
			readJackModels(j, flags, keyName::NodeName_JackOutputDescriptors, c.nextExpectedJackOutputIndex, config.jackOutputTrees, c, ignoreDynamicModel);

			// Read AvbInterfaces
			// Will be a tree in 1722.1-2021
			readLeafModels<false, false, true>(j, flags, keyName::NodeName_AvbInterfaceDescriptors, c.nextExpectedAvbInterfaceIndex, config.avbInterfaceModels, ignoreDynamicModel);

			// Read PtpInstances
			readPtpInstanceModels(j, flags, keyName::NodeName_PtpInstanceDescriptors, c.nextExpectedPtpInstanceIndex, config.ptpInstanceTrees, c, ignoreDynamicModel);
		}

		// Legacy dump file support, we must build the descriptor counts
		if (mustRebuildDescriptorCount)
		{
			auto& counts = config.staticModel.descriptorCounts;
			setDescriptorCount(counts, DescriptorType::AudioUnit, config.audioUnitTrees);
			setDescriptorCount(counts, DescriptorType::StreamInput, config.streamInputModels);
			setDescriptorCount(counts, DescriptorType::StreamOutput, config.streamOutputModels);
			setDescriptorCount(counts, DescriptorType::JackInput, config.jackInputTrees);
			setDescriptorCount(counts, DescriptorType::JackOutput, config.jackOutputTrees);
			setDescriptorCount(counts, DescriptorType::AvbInterface, config.avbInterfaceModels);
			setDescriptorCount(counts, DescriptorType::ClockSource, config.clockSourceModels);
			setDescriptorCount(counts, DescriptorType::Control, config.controlModels);
			setDescriptorCount(counts, DescriptorType::Locale, config.localeTrees);
			setDescriptorCount(counts, DescriptorType::MemoryObject, config.memoryObjectModels);
			setDescriptorCount(counts, DescriptorType::ClockDomain, config.clockDomainModels);
			setDescriptorCount(counts, DescriptorType::Timing, config.timingModels);
			setDescriptorCount(counts, DescriptorType::PtpInstance, config.ptpInstanceTrees);
		}

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
