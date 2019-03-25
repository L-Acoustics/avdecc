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
* @file avdeccControlledEntityJsonSerializer.cpp
* @author Christophe Calmejane
*/

#include "avdeccControlledEntityJsonSerializer.hpp"
#include "la/avdecc/internals/jsonTypes.hpp"
#include "la/avdecc/controller/internals/jsonTypes.hpp"

using json = nlohmann::json;

namespace la
{
namespace avdecc
{
namespace controller
{
namespace entitySerializer
{
class Visitor : public model::EntityModelVisitor
{
public:
	Visitor(json& object)
		: _object(object)
	{
	}

	bool operator!() const noexcept
	{
		return !_serializationError.empty();
	}

	std::string const& getSerializationError() const noexcept
	{
		return _serializationError;
	}

	// Defaulted compiler auto-generated methods
	virtual ~Visitor() noexcept = default;
	Visitor(Visitor&&) = default;
	Visitor(Visitor const&) = default;
	Visitor& operator=(Visitor const&) = default;
	Visitor& operator=(Visitor&&) = default;

private:
	// model::EntityModelVisitor overrides
	virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const& node) noexcept override
	{
		auto& jnode = _object[keyName::ControlledEntity_EntityDescriptor];

		// Static model
		if (node.staticModel)
		{
			auto const& s = *node.staticModel;
			auto jstatic = json{};

			jstatic[model::keyName::EntityNode_Static_VendorNameString] = s.vendorNameString;
			jstatic[model::keyName::EntityNode_Static_ModelNameString] = s.modelNameString;

			jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
		}

		// Dynamic model
		if (node.dynamicModel)
		{
			auto const& d = *node.dynamicModel;
			auto jdynamic = json{};

			jdynamic[model::keyName::EntityNode_Dynamic_EntityName] = d.entityName;
			jdynamic[model::keyName::EntityNode_Dynamic_GroupName] = d.groupName;
			jdynamic[model::keyName::EntityNode_Dynamic_FirmwareVersion] = d.firmwareVersion;
			jdynamic[model::keyName::EntityNode_Dynamic_SerialNumber] = d.serialNumber;
			jdynamic[model::keyName::EntityNode_Dynamic_CurrentConfiguration] = d.currentConfiguration;

			jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const /*parent*/, model::ConfigurationNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto const& jparentIt = _object.find(keyName::ControlledEntity_EntityDescriptor);
		if (!AVDECC_ASSERT_WITH_RET(jparentIt != _object.end(), "JSON parent node not found"))
		{
			_serializationError = "JSON parent node not found (Entity)";
			return;
		}

		// Validate index
		if (_nextExpectedConfigurationIndex != node.descriptorIndex)
		{
			_serializationError = "ConfigurationIndex is not the expected one";
			return;
		}

		// Update next expected index
		++_nextExpectedConfigurationIndex;

		// Create JSON node
		auto& jconfigs = jparentIt->operator[](keyName::ControlledEntity_ConfigurationDescriptors);
		auto jnode = json{};

		jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

		// Static model
		if (node.staticModel)
		{
			auto const& s = *node.staticModel;
			auto jstatic = json{};

			jstatic[model::keyName::ConfigurationNode_Static_LocalizedDescription] = s.localizedDescription;

			jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
		}

		// Dynamic model
		if (node.dynamicModel)
		{
			auto const& d = *node.dynamicModel;
			auto jdynamic = json{};

			jdynamic[model::keyName::ConfigurationNode_Dynamic_ObjectName] = d.objectName;

			jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
		}

		// Save the node
		jconfigs.push_back(jnode);
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AudioUnitNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedAudioUnitIndex != node.descriptorIndex)
			{
				_serializationError = "AudioUnitIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedAudioUnitIndex;

			// Create JSON node
			auto& junits = jconfig[keyName::ControlledEntity_AudioUnitDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				auto const& s = *node.staticModel;
				auto jstatic = json{};

				jstatic[model::keyName::AudioUnitNode_Static_LocalizedDescription] = s.localizedDescription;
				jstatic[model::keyName::AudioUnitNode_Static_ClockDomainIndex] = s.clockDomainIndex;
				jstatic[model::keyName::AudioUnitNode_Static_SamplingRates] = s.samplingRates;

				jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::AudioUnitNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::AudioUnitNode_Dynamic_CurrentSamplingRate] = d.currentSamplingRate;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			junits.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	json JsonizeStreamNodeStaticModel(model::StreamNodeStaticModel const& model)
	{
		auto jstatic = json{};

		jstatic[model::keyName::StreamNode_Static_LocalizedDescription] = model.localizedDescription;
		jstatic[model::keyName::StreamNode_Static_ClockDomainIndex] = model.clockDomainIndex;
		jstatic[model::keyName::StreamNode_Static_StreamFlags] = model.streamFlags;
		jstatic[model::keyName::StreamNode_Static_BackupTalkerEntityID0] = model.backupTalkerEntityID_0;
		jstatic[model::keyName::StreamNode_Static_BackupTalkerUniqueID0] = model.backupTalkerUniqueID_0;
		jstatic[model::keyName::StreamNode_Static_BackupTalkerEntityID1] = model.backupTalkerEntityID_1;
		jstatic[model::keyName::StreamNode_Static_BackupTalkerUniqueID1] = model.backupTalkerUniqueID_1;
		jstatic[model::keyName::StreamNode_Static_BackupTalkerEntityID2] = model.backupTalkerEntityID_2;
		jstatic[model::keyName::StreamNode_Static_BackupTalkerUniqueID2] = model.backupTalkerUniqueID_2;
		jstatic[model::keyName::StreamNode_Static_BackedupTalkerEntityID] = model.backedupTalkerEntityID;
		jstatic[model::keyName::StreamNode_Static_BackedupTalkerUnique] = model.backedupTalkerUnique;
		jstatic[model::keyName::StreamNode_Static_AvbInterfaceIndex] = model.avbInterfaceIndex;
		jstatic[model::keyName::StreamNode_Static_BufferLength] = model.bufferLength;
		jstatic[model::keyName::StreamNode_Static_Formats] = model.formats;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		if (!model.redundantStreams.empty())
		{
			jstatic[model::keyName::StreamNode_Static_RedundantStreams] = model.redundantStreams;
		}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

		return jstatic;
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamInputNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedStreamInputIndex != node.descriptorIndex)
			{
				_serializationError = "StreamInputIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedStreamInputIndex;

			// Create JSON node
			auto& jstreams = jconfig[keyName::ControlledEntity_StreamInputDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				jnode[keyName::ControlledEntity_StaticInformation] = JsonizeStreamNodeStaticModel(*node.staticModel);
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::StreamInputNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::StreamInputNode_Dynamic_StreamInfo] = d.streamInfo;
				jdynamic[model::keyName::StreamInputNode_Dynamic_ConnectedTalker] = d.connectionState.talkerStream;
				jdynamic[model::keyName::StreamInputNode_Dynamic_Counters] = d.counters;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			jstreams.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamOutputNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedStreamOutputIndex != node.descriptorIndex)
			{
				_serializationError = "StreamOutputIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedStreamOutputIndex;

			// Create JSON node
			auto& jstreams = jconfig[keyName::ControlledEntity_StreamOutputDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				jnode[keyName::ControlledEntity_StaticInformation] = JsonizeStreamNodeStaticModel(*node.staticModel);
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::StreamOutputNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::StreamOutputNode_Dynamic_StreamInfo] = d.streamInfo;
				jdynamic[model::keyName::StreamOutputNode_Dynamic_Counters] = d.counters;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			jstreams.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AvbInterfaceNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedAvbInterfaceIndex != node.descriptorIndex)
			{
				_serializationError = "AvbInterfaceIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedAvbInterfaceIndex;

			// Create JSON node
			auto& jinterfaces = jconfig[keyName::ControlledEntity_AvbInterfaceDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				auto const& s = *node.staticModel;
				auto jstatic = json{};

				jstatic[model::keyName::AvbInterfaceNode_Static_LocalizedDescription] = s.localizedDescription;
				jstatic[model::keyName::AvbInterfaceNode_Static_MacAddress] = networkInterface::macAddressToString(s.macAddress, true);
				jstatic[model::keyName::AvbInterfaceNode_Static_Flags] = s.interfaceFlags;
				jstatic[model::keyName::AvbInterfaceNode_Static_ClockIdentity] = s.clockIdentity;
				jstatic[model::keyName::AvbInterfaceNode_Static_Priority1] = s.priority1;
				jstatic[model::keyName::AvbInterfaceNode_Static_ClockClass] = s.clockClass;
				jstatic[model::keyName::AvbInterfaceNode_Static_OffsetScaledLogVariance] = s.offsetScaledLogVariance;
				jstatic[model::keyName::AvbInterfaceNode_Static_ClockAccuracy] = s.clockAccuracy;
				jstatic[model::keyName::AvbInterfaceNode_Static_Priority2] = s.priority2;
				jstatic[model::keyName::AvbInterfaceNode_Static_DomainNumber] = s.domainNumber;
				jstatic[model::keyName::AvbInterfaceNode_Static_LogSyncInterval] = s.logSyncInterval;
				jstatic[model::keyName::AvbInterfaceNode_Static_LogAnnounceInterval] = s.logAnnounceInterval;
				jstatic[model::keyName::AvbInterfaceNode_Static_LogPdelayInterval] = s.logPDelayInterval;
				jstatic[model::keyName::AvbInterfaceNode_Static_PortNumber] = s.portNumber;

				jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::AvbInterfaceNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::AvbInterfaceNode_Dynamic_AvbInfo] = d.avbInfo;
				jdynamic[model::keyName::AvbInterfaceNode_Dynamic_AsPath] = d.asPath;
				jdynamic[model::keyName::AvbInterfaceNode_Dynamic_Counters] = d.counters;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			jinterfaces.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedClockSourceIndex != node.descriptorIndex)
			{
				_serializationError = "ClockSourceIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedClockSourceIndex;

			// Create JSON node
			auto& jsources = jconfig[keyName::ControlledEntity_ClockSourceDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				auto const& s = *node.staticModel;
				auto jstatic = json{};

				jstatic[model::keyName::ClockSourceNode_Static_LocalizedDescription] = s.localizedDescription;
				jstatic[model::keyName::ClockSourceNode_Static_ClockSourceType] = s.clockSourceType;
				jstatic[model::keyName::ClockSourceNode_Static_ClockSourceLocationType] = s.clockSourceLocationType;
				jstatic[model::keyName::ClockSourceNode_Static_ClockSourceLocationIndex] = s.clockSourceLocationIndex;

				jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::ClockSourceNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::ClockSourceNode_Dynamic_ClockSourceFlags] = d.clockSourceFlags;
				jdynamic[model::keyName::ClockSourceNode_Dynamic_ClockSourceIdentifier] = d.clockSourceIdentifier;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			jsources.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedMemoryObjectIndex != node.descriptorIndex)
			{
				_serializationError = "MemoryObjectIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedMemoryObjectIndex;

			// Create JSON node
			auto& jobjects = jconfig[keyName::ControlledEntity_MemoryObjectDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				auto const& s = *node.staticModel;
				auto jstatic = json{};

				jstatic[model::keyName::MemoryObjectNode_Static_LocalizedDescription] = s.localizedDescription;
				jstatic[model::keyName::MemoryObjectNode_Static_MemoryObjectType] = s.memoryObjectType;
				jstatic[model::keyName::MemoryObjectNode_Static_TargetDescriptorType] = s.targetDescriptorType;
				jstatic[model::keyName::MemoryObjectNode_Static_TargetDescriptorIndex] = s.targetDescriptorIndex;
				jstatic[model::keyName::MemoryObjectNode_Static_StartAddress] = utils::toHexString(s.startAddress, true, true);
				jstatic[model::keyName::MemoryObjectNode_Static_MaximumLength] = s.maximumLength;

				jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::MemoryObjectNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::MemoryObjectNode_Dynamic_Length] = d.length;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			jobjects.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::LocaleNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedLocaleIndex != node.descriptorIndex)
			{
				_serializationError = "LocaleIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedLocaleIndex;

			// Create JSON node
			auto& jlocales = jconfig[keyName::ControlledEntity_LocaleDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				auto const& s = *node.staticModel;
				auto jstatic = json{};

				jstatic[model::keyName::LocaleNode_Static_LocaleID] = s.localeID;
				jstatic[model::keyName::LocaleNode_Static_BaseStringDescriptor] = s.baseStringDescriptorIndex;

				jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
			}

			// Save the node
			jlocales.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::LocaleNode const* const parent, model::StringsNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON grand-parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(grandParent->descriptorIndex);

			// Search the JSON parent
			auto& jlocales = jconfig[keyName::ControlledEntity_LocaleDescriptors];
			try
			{
				auto& jlocale = jlocales.at(parent->descriptorIndex);

				// Validate index
				if (_nextExpectedStringsIndex != node.descriptorIndex)
				{
					_serializationError = "StringsIndex is not the expected one";
					return;
				}

				// Update next expected index
				++_nextExpectedStringsIndex;

				// Create JSON node
				auto& jstrings = jlocale[keyName::ControlledEntity_StringsDescriptors];
				auto jnode = json{};

				jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

				// Static model
				if (node.staticModel)
				{
					auto const& s = *node.staticModel;
					auto jstatic = json{};

					jstatic[model::keyName::StringsNode_Static_Strings] = s.strings;

					jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
				}

				// Save the node
				jstrings.push_back(jnode);
			}
			catch (json::exception const&)
			{
				_serializationError = "JSON parent node not found (Locale)";
				return;
			}
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON grand-parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON grand-parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(grandParent->descriptorIndex);

			// Search the JSON parent
			auto& junits = jconfig[keyName::ControlledEntity_AudioUnitDescriptors];
			try
			{
				auto& junit = junits.at(parent->descriptorIndex);

				// Validate index
				auto& nextExpectedIndex = node.descriptorType == entity::model::DescriptorType::StreamPortInput ? _nextExpectedStreamPortInputIndex : _nextExpectedStreamPortOutputIndex;
				if (nextExpectedIndex != node.descriptorIndex)
				{
					_serializationError = "StreamPortIndex is not the expected one";
					return;
				}

				// Update next expected index
				++nextExpectedIndex;

				// Create JSON node
				auto const streamPortTypeName = node.descriptorType == entity::model::DescriptorType::StreamPortInput ? "stream_port_input_descriptors" : "stream_port_output_descriptors";
				auto& jports = junit[streamPortTypeName];
				auto jnode = json{};

				jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

				// Static model
				if (node.staticModel)
				{
					auto const& s = *node.staticModel;
					auto jstatic = json{};

					jstatic[model::keyName::StreamPortNode_Static_ClockDomainIndex] = s.clockDomainIndex;
					jstatic[model::keyName::StreamPortNode_Static_Flags] = s.portFlags;

					jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
				}

				// Dynamic model
				if (node.dynamicModel)
				{
					auto const& d = *node.dynamicModel;
					auto jdynamic = json{};

					if (node.staticModel && node.staticModel->hasDynamicAudioMap)
					{
						jdynamic[model::keyName::StreamPortNode_Dynamic_DynamicMappings] = d.dynamicAudioMap;
					}

					jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
				}

				// Save the node
				jports.push_back(jnode);
			}
			catch (json::exception const&)
			{
				_serializationError = "JSON parent node not found (AudioUnit)";
				return;
			}
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON grand-parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioClusterNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON grand-grand-parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(grandGrandParent->descriptorIndex);

			// Search the JSON grand-parent
			auto& junits = jconfig[keyName::ControlledEntity_AudioUnitDescriptors];
			try
			{
				auto& junit = junits.at(grandParent->descriptorIndex);

				// Search the JSON parent
				auto const streamPortTypeName = parent->descriptorType == entity::model::DescriptorType::StreamPortInput ? "stream_port_input_descriptors" : "stream_port_output_descriptors";
				auto& jports = junit[streamPortTypeName];
				try
				{
					auto& jport = jports.at(parent->descriptorIndex);

					// Validate index
					if (_nextExpectedAudioClusterIndex != node.descriptorIndex)
					{
						_serializationError = "AudioClusterIndex is not the expected one";
						return;
					}

					// Update next expected index
					++_nextExpectedAudioClusterIndex;

					// Create JSON node
					auto& jclusters = jport[keyName::ControlledEntity_AudioClusterDescriptors];
					auto jnode = json{};

					jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

					// Static model
					if (node.staticModel)
					{
						auto const& s = *node.staticModel;
						auto jstatic = json{};

						jstatic[model::keyName::AudioClusterNode_Static_LocalizedDescription] = s.localizedDescription;
						jstatic[model::keyName::AudioClusterNode_Static_SignalType] = s.signalType;
						jstatic[model::keyName::AudioClusterNode_Static_SignalIndex] = s.signalIndex;
						jstatic[model::keyName::AudioClusterNode_Static_SignalOutput] = s.signalOutput;
						jstatic[model::keyName::AudioClusterNode_Static_PathLatency] = s.pathLatency;
						jstatic[model::keyName::AudioClusterNode_Static_BlockLatency] = s.blockLatency;
						jstatic[model::keyName::AudioClusterNode_Static_ChannelCount] = s.channelCount;
						jstatic[model::keyName::AudioClusterNode_Static_Format] = s.format;

						jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
					}

					// Dynamic model
					if (node.dynamicModel)
					{
						auto const& d = *node.dynamicModel;
						auto jdynamic = json{};

						jdynamic[model::keyName::AudioClusterNode_Dynamic_ObjectName] = d.objectName;

						jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
					}

					// Save the node
					jclusters.push_back(jnode);
				}
				catch (json::exception const&)
				{
					_serializationError = "JSON parent node not found (StreamPort)";
					return;
				}
			}
			catch (json::exception const&)
			{
				_serializationError = "JSON parent node not found (AudioUnit)";
				return;
			}
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON grand-parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioMapNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON grand-grand-parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(grandGrandParent->descriptorIndex);

			// Search the JSON grand-parent
			auto& junits = jconfig[keyName::ControlledEntity_AudioUnitDescriptors];
			try
			{
				auto& junit = junits.at(grandParent->descriptorIndex);

				// Search the JSON parent
				auto const streamPortTypeName = parent->descriptorType == entity::model::DescriptorType::StreamPortInput ? "stream_port_input_descriptors" : "stream_port_output_descriptors";
				auto& jports = junit[streamPortTypeName];
				try
				{
					auto& jport = jports.at(parent->descriptorIndex);

					// Validate index
					if (_nextExpectedAudioMapIndex != node.descriptorIndex)
					{
						_serializationError = "AudioMapIndex is not the expected one";
						return;
					}

					// Update next expected index
					++_nextExpectedAudioMapIndex;

					// Create JSON node
					auto& jmaps = jport[keyName::ControlledEntity_AudioMapDescriptors];
					auto jnode = json{};

					jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

					// Static model
					if (node.staticModel)
					{
						auto const& s = *node.staticModel;
						auto jstatic = json{};

						jstatic[model::keyName::AudioMapNode_Static_Mappings] = s.mappings;

						jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
					}

					// Save the node
					jmaps.push_back(jnode);
				}
				catch (json::exception const&)
				{
					_serializationError = "JSON parent node not found (StreamPort)";
					return;
				}
			}
			catch (json::exception const&)
			{
				_serializationError = "JSON parent node not found (AudioUnit)";
				return;
			}
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON grand-parent node not found (Configuration)";
			return;
		}
	}

	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockDomainNode const& node) noexcept override
	{
		// Check no previous error
		if (!_serializationError.empty())
		{
			return;
		}

		// Search the JSON parent
		auto& jconfigs = _object[keyName::ControlledEntity_EntityDescriptor][keyName::ControlledEntity_ConfigurationDescriptors];
		try
		{
			auto& jconfig = jconfigs.at(parent->descriptorIndex);

			// Validate index
			if (_nextExpectedClockDomainIndex != node.descriptorIndex)
			{
				_serializationError = "ClockDomainIndex is not the expected one";
				return;
			}

			// Update next expected index
			++_nextExpectedClockDomainIndex;

			// Create JSON node
			auto& jdomains = jconfig[keyName::ControlledEntity_ClockDomainDescriptors];
			auto jnode = json{};

			jnode[model::keyName::Node_Informative_Index] = node.descriptorIndex;

			// Static model
			if (node.staticModel)
			{
				auto const& s = *node.staticModel;
				auto jstatic = json{};

				jstatic[model::keyName::ClockDomainNode_Static_LocalizedDescription] = s.localizedDescription;
				jstatic[model::keyName::ClockDomainNode_Static_ClockSources] = s.clockSources;

				jnode[keyName::ControlledEntity_StaticInformation] = jstatic;
			}

			// Dynamic model
			if (node.dynamicModel)
			{
				auto const& d = *node.dynamicModel;
				auto jdynamic = json{};

				jdynamic[model::keyName::ClockDomainNode_Dynamic_ObjectName] = d.objectName;
				jdynamic[model::keyName::ClockDomainNode_Dynamic_ClockSourceIndex] = d.clockSourceIndex;
				jdynamic[model::keyName::ClockDomainNode_Dynamic_Counters] = d.counters;

				jnode[keyName::ControlledEntity_DynamicInformation] = jdynamic;
			}

			// Save the node
			jdomains.push_back(jnode);
		}
		catch (json::exception const&)
		{
			_serializationError = "JSON parent node not found (Configuration)";
			return;
		}
	}

	// Private members
	json& _object; // NSDMI not possible for a reference
	std::string _serializationError{};
	entity::model::ConfigurationIndex _nextExpectedConfigurationIndex{ 0u };
	entity::model::AudioUnitIndex _nextExpectedAudioUnitIndex{ 0u };
	entity::model::StreamIndex _nextExpectedStreamInputIndex{ 0u };
	entity::model::StreamIndex _nextExpectedStreamOutputIndex{ 0u };
	entity::model::AvbInterfaceIndex _nextExpectedAvbInterfaceIndex{ 0u };
	entity::model::ClockSourceIndex _nextExpectedClockSourceIndex{ 0u };
	entity::model::MemoryObjectIndex _nextExpectedMemoryObjectIndex{ 0u };
	entity::model::LocaleIndex _nextExpectedLocaleIndex{ 0u };
	entity::model::StringsIndex _nextExpectedStringsIndex{ 0u };
	entity::model::StreamPortIndex _nextExpectedStreamPortInputIndex{ 0u };
	entity::model::StreamPortIndex _nextExpectedStreamPortOutputIndex{ 0u };
	entity::model::ClusterIndex _nextExpectedAudioClusterIndex{ 0u };
	entity::model::MapIndex _nextExpectedAudioMapIndex{ 0u };
	entity::model::ClockDomainIndex _nextExpectedClockDomainIndex{ 0u };
};


json createJsonObject(ControlledEntity const& entity) noexcept
{
	// Create the object
	auto object = json{};
	auto const& e = entity.getEntity();

	// Dump information of the dump itself
	object[keyName::ControlledEntity_DumpVersion] = 1;

	// Dump device compatibility flags
	object[keyName::ControlledEntity_CompatibilityFlags] = entity.getCompatibilityFlags();

	// Dump ADP information
	{
		auto& adp = object[keyName::ControlledEntity_AdpInformation];

		// Dump common information
		adp[entity::keyName::Entity_CommonInformation_Node] = e.getCommonInformation();

		// Dump interfaces information
		for (auto const& [avbInterfaceIndex, intfcInfo] : e.getInterfacesInformation())
		{
			json j = intfcInfo; // Must use operator= instead of constructor to force usage of the to_json overload
			if (avbInterfaceIndex == entity::Entity::GlobalAvbInterfaceIndex)
			{
				j[entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex] = nullptr;
			}
			else
			{
				j[entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex] = avbInterfaceIndex;
			}
			object[entity::keyName::Entity_InterfaceInformation_Node].push_back(j);
		}
	}

	// Dump AEM if supported
	if (e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
	{
		auto& j = object[keyName::ControlledEntity_EntityModel];
		auto v = Visitor{ j };
		entity.accept(&v);
		if (!v)
		{
			return json{ v.getSerializationError() };
		}
	}

	// Dump Milan information, if present
	auto const milanInfo = entity.getMilanInfo();
	if (milanInfo)
	{
		object[keyName::ControlledEntity_MilanInformation] = *milanInfo;
	}

	// Entity State
	{
		auto& state = object[keyName::ControlledEntity_EntityState];
		state[controller::keyName::ControlledEntityState_AcquireState] = entity.getAcquireState();
		state[controller::keyName::ControlledEntityState_OwningControllerID] = entity.getOwningControllerID();
		state[controller::keyName::ControlledEntityState_LockState] = entity.getLockState();
		state[controller::keyName::ControlledEntityState_LockingControllerID] = entity.getLockingControllerID();
		state[controller::keyName::ControlledEntityState_SubscribedUnsol] = entity.isSubscribedToUnsolicitedNotifications();
	}

	return object;
}

} // namespace entitySerializer
} // namespace controller
} // namespace avdecc
} // namespace la
