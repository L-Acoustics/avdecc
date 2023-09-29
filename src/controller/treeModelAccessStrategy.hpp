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
* @file treeModelAccessStrategy.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
#include "avdeccControllerLogHelper.hpp"

#include <la/avdecc/internals/entityModelTree.hpp>
#include <la/avdecc/utils.hpp>

#include <memory>

namespace la
{
namespace avdecc
{
namespace controller
{
class ControlledEntityImpl;

class TreeModelAccessStrategy
{
public:
	using UniquePointer = std::unique_ptr<TreeModelAccessStrategy>;
	enum class StrategyType
	{
		Unknown = 0,
		Traverse,
		Cached
	};
	enum class NotFoundBehavior
	{
		IgnoreAndReturnNull = 0, /**< Will silently ignore and return a nullptr value */
		LogAndReturnNull, /**< Will log an error and return a nullptr value */
		DefaultConstruct, /**< Will default construct the model and return it */
		Throw, /**< Will throw an Exception */
	};
	/** Hierarchy Hint for the DefaultConstruct behavior for Descriptors that can be found at multiple levels (eg. Controls) */
	enum class DefaultConstructLevelHint
	{
		None = 0,
		Configuration,
		AudioUnit,
		StreamPortInput,
		StreamPortOutput,
		JackInput,
		JackOutput,
		AvbInterface,
		PtpInstance,
	};

protected:
	virtual UniqueIdentifier getEntityID() const noexcept = 0;

	TreeModelAccessStrategy(ControlledEntityImpl* const entity) noexcept
		: _entity{ entity }
	{
	}

	bool handleDescriptorNotFound(NotFoundBehavior const notFoundBehavior, ControlledEntity::Exception::Type const exceptionType, std::string const& message)
	{
		switch (notFoundBehavior)
		{
			case NotFoundBehavior::IgnoreAndReturnNull:
				break;
			case NotFoundBehavior::LogAndReturnNull:
				LOG_CONTROLLER_DEBUG(getEntityID(), message.c_str());
				break;
			case NotFoundBehavior::DefaultConstruct:
				return true;
			case NotFoundBehavior::Throw:
				throw ControlledEntity::Exception(exceptionType, message.c_str());
			default:
				break;
		}
		return false;
	}

	template<typename TreeModelAccessPointer, typename DescriptorIndexType, typename... Parameters>
	auto* getNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const descriptorIndex, TreeModelAccessPointer TreeModelAccessStrategy::*Pointer, Parameters&&... params)
	{
		auto* const node = (this->*Pointer)(configurationIndex, descriptorIndex, std::forward<Parameters>(params)...);
		if (node)
		{
			return &(node->staticModel);
		}
		return decltype(&node->staticModel){ nullptr };
	}

	template<typename TreeModelAccessPointer, typename DescriptorIndexType, typename... Parameters>
	auto* getNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, DescriptorIndexType const descriptorIndex, TreeModelAccessPointer TreeModelAccessStrategy::*Pointer, Parameters&&... params)
	{
		auto* const node = (this->*Pointer)(configurationIndex, descriptorIndex, std::forward<Parameters>(params)...);
		if (node)
		{
			return &(node->dynamicModel);
		}
		return decltype(&node->dynamicModel){ nullptr };
	}

	template<typename DescriptorIndexType>
	bool isDescriptorIndexInRange(DescriptorIndexType const descriptorIndex, DescriptorIndexType const baseIndex, std::uint16_t const countDescriptors)
	{
		return descriptorIndex >= baseIndex && descriptorIndex < static_cast<DescriptorIndexType>(baseIndex + countDescriptors);
	}

	ControlledEntityImpl* _entity{ nullptr };

public:
	virtual StrategyType getStrategyType() const noexcept = 0;
	virtual model::EntityNode* getEntityNode(NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::EntityNodeStaticModel* getEntityNodeStaticModel(NotFoundBehavior const notFoundBehavior)
	{
		auto* const entityNode = getEntityNode(notFoundBehavior);
		if (entityNode)
		{
			return &(entityNode->staticModel);
		}
		return nullptr;
	}
	virtual entity::model::EntityNodeDynamicModel* getEntityNodeDynamicModel(NotFoundBehavior const notFoundBehavior)
	{
		auto* const entityNode = getEntityNode(notFoundBehavior);
		if (entityNode)
		{
			return &(entityNode->dynamicModel);
		}
		return nullptr;
	}

	virtual model::ConfigurationNode* getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::ConfigurationNodeStaticModel* getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, NotFoundBehavior const notFoundBehavior)
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			return &(configurationNode->staticModel);
		}
		return nullptr;
	}
	virtual entity::model::ConfigurationNodeDynamicModel* getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, NotFoundBehavior const notFoundBehavior)
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			return &(configurationNode->dynamicModel);
		}
		return nullptr;
	}

	virtual model::AudioUnitNode* getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::AudioUnitNodeStaticModel* getAudioUnitNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAudioUnitNode, notFoundBehavior);
	}
	virtual entity::model::AudioUnitNodeDynamicModel* getAudioUnitNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAudioUnitNode, notFoundBehavior);
	}

	//virtual model::VideoUnitNode* getVideoUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::VideoUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::VideoUnitNodeStaticModel* getVideoUnitNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::VideoUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::VideoUnitNodeDynamicModel* getVideoUnitNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::VideoUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SensorUnitNode* getSensorUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SensorUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SensorUnitNodeStaticModel* getSensorUnitNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SensorUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SensorUnitNodeDynamicModel* getSensorUnitNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SensorUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::StreamInputNode* getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual model::StreamOutputNode* getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::StreamNodeStaticModel* getStreamInputNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamInputNode, notFoundBehavior);
	}
	virtual entity::model::StreamNodeStaticModel* getStreamOutputNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamOutputNode, notFoundBehavior);
	}
	virtual entity::model::StreamInputNodeDynamicModel* getStreamInputNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamInputNode, notFoundBehavior);
	}
	virtual entity::model::StreamOutputNodeDynamicModel* getStreamOutputNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamOutputNode, notFoundBehavior);
	}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::RedundantStreamInputNode* getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual model::RedundantStreamOutputNode* getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex, NotFoundBehavior const notFoundBehavior) = 0;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	virtual model::JackInputNode* getJackInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual model::JackOutputNode* getJackOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::JackNodeStaticModel* getJackInputNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getJackInputNode, notFoundBehavior);
	}
	virtual entity::model::JackNodeStaticModel* getJackOutputNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getJackOutputNode, notFoundBehavior);
	}
	virtual entity::model::JackNodeDynamicModel* getJackInputNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getJackInputNode, notFoundBehavior);
	}
	virtual entity::model::JackNodeDynamicModel* getJackOutputNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getJackOutputNode, notFoundBehavior);
	}

	virtual model::AvbInterfaceNode* getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::AvbInterfaceNodeStaticModel* getAvbInterfaceNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAvbInterfaceNode, notFoundBehavior);
	}
	virtual entity::model::AvbInterfaceNodeDynamicModel* getAvbInterfaceNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAvbInterfaceNode, notFoundBehavior);
	}

	virtual model::ClockSourceNode* getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::ClockSourceNodeStaticModel* getClockSourceNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getClockSourceNode, notFoundBehavior);
	}
	virtual entity::model::ClockSourceNodeDynamicModel* getClockSourceNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getClockSourceNode, notFoundBehavior);
	}

	virtual model::MemoryObjectNode* getMemoryObjectNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::MemoryObjectNodeStaticModel* getMemoryObjectNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getMemoryObjectNode, notFoundBehavior);
	}
	virtual entity::model::MemoryObjectNodeDynamicModel* getMemoryObjectNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getMemoryObjectNode, notFoundBehavior);
	}

	virtual model::LocaleNode* getLocaleNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::LocaleNodeStaticModel* getLocaleNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getLocaleNode, notFoundBehavior);
	}
	//virtual entity::model::LocaleNodeDynamicModel* getLocaleNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::StringsNode* getStringsNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::StringsNodeStaticModel* getStringsNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStringsNode, notFoundBehavior);
	}
	//virtual entity::model::StringsNodeDynamicModel* getStringsNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::StreamPortInputNode* getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual model::StreamPortOutputNode* getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::StreamPortNodeStaticModel* getStreamPortInputNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamPortInputNode, notFoundBehavior);
	}
	virtual entity::model::StreamPortNodeStaticModel* getStreamPortOutputNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamPortOutputNode, notFoundBehavior);
	}
	virtual entity::model::StreamPortNodeDynamicModel* getStreamPortInputNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamPortInputNode, notFoundBehavior);
	}
	virtual entity::model::StreamPortNodeDynamicModel* getStreamPortOutputNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getStreamPortOutputNode, notFoundBehavior);
	}

	//virtual model::ExternalPortNode* getExternalPortNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::ExternalPortNodeStaticModel* getExternalPortNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::ExternalPortNodeDynamicModel* getExternalPortNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::InternalPortNode* getInternalPortNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::InternalPortNodeStaticModel* getInternalPortNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::InternalPortNodeDynamicModel* getInternalPortNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::AudioClusterNode* getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::AudioClusterNodeStaticModel* getAudioClusterNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAudioClusterNode, notFoundBehavior);
	}
	virtual entity::model::AudioClusterNodeDynamicModel* getAudioClusterNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAudioClusterNode, notFoundBehavior);
	}

	//virtual model::VideoClusterNode* getVideoClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::VideoClusterNodeStaticModel* getVideoClusterNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::VideoClusterNodeDynamicModel* getVideoClusterNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SensorClusterNode* getSensorClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SensorClusterNodeStaticModel* getSensorClusterNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SensorClusterNodeDynamicModel* getSensorClusterNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::AudioMapNode* getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::AudioMapNodeStaticModel* getAudioMapNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getAudioMapNode, notFoundBehavior);
	}
	//virtual entity::model::AudioMapNodeDynamicModel* getAudioMapNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::VideoMapNode* getVideoMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::VideoMapNodeStaticModel* getVideoMapNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::VideoMapNodeDynamicModel* getVideoMapNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SensorMapNode* getSensorMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SensorMapNodeStaticModel* getSensorMapNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SensorMapNodeDynamicModel* getSensorMapNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::ControlNode* getControlNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior, DefaultConstructLevelHint const levelHint) = 0;
	virtual entity::model::ControlNodeStaticModel* getControlNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getControlNode, notFoundBehavior, DefaultConstructLevelHint::None);
	}
	virtual entity::model::ControlNodeDynamicModel* getControlNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getControlNode, notFoundBehavior, DefaultConstructLevelHint::None);
	}

	//virtual model::SignalSelectorNode* getSignalSelectorNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalSelectorIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalSelectorNodeStaticModel* getSignalSelectorNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalSelectorIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalSelectorNodeDynamicModel* getSignalSelectorNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalSelectorIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::MixerNode* getMixerNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MixerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::MixerNodeStaticModel* getMixerNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MixerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::MixerNodeDynamicModel* getMixerNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MixerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::MatrixNode* getMatrixNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MatrixIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::MatrixNodeStaticModel* getMatrixNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MatrixIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::MatrixNodeDynamicModel* getMatrixNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MatrixIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::MatrixSignalNode* getMatrixSignalNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MatrixSignalIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::MatrixSignalNodeStaticModel* getMatrixSignalNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MatrixSignalIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::MatrixSignalNodeDynamicModel* getMatrixSignalNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::MatrixSignalIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SignalSplitterNode* getSignalSplitterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalSplitterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalSplitterNodeStaticModel* getSignalSplitterNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalSplitterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalSplitterNodeDynamicModel* getSignalSplitterNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalSplitterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SignalCombinerNode* getSignalCombinerNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalCombinerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalCombinerNodeStaticModel* getSignalCombinerNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalCombinerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalCombinerNodeDynamicModel* getSignalCombinerNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalCombinerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SignalDemultiplexerNode* getSignalDemultiplexerNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalDemultiplexerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalDemultiplexerNodeStaticModel* getSignalDemultiplexerNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalDemultiplexerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalDemultiplexerNodeDynamicModel* getSignalDemultiplexerNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalDemultiplexerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SignalMultiplexerNode* getSignalMultiplexerNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalMultiplexerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalMultiplexerNodeStaticModel* getSignalMultiplexerNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalMultiplexerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalMultiplexerNodeDynamicModel* getSignalMultiplexerNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalMultiplexerIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	//virtual model::SignalTranscoderNode* getSignalTranscoderNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalTranscoderIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalTranscoderNodeStaticModel* getSignalTranscoderNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalTranscoderIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::SignalTranscoderNodeDynamicModel* getSignalTranscoderNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::SignalTranscoderIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::ClockDomainNode* getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::ClockDomainNodeStaticModel* getClockDomainNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getClockDomainNode, notFoundBehavior);
	}
	virtual entity::model::ClockDomainNodeDynamicModel* getClockDomainNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getClockDomainNode, notFoundBehavior);
	}

	//virtual model::ControlBlockNode* getControlBlockNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlBlockIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::ControlBlockNodeStaticModel* getControlBlockNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlBlockIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	//virtual entity::model::ControlBlockNodeDynamicModel* getControlBlockNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlBlockIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;

	virtual model::TimingNode* getTimingNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::TimingNodeStaticModel* getTimingNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getTimingNode, notFoundBehavior);
	}
	virtual entity::model::TimingNodeDynamicModel* getTimingNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getTimingNode, notFoundBehavior);
	}

	virtual model::PtpInstanceNode* getPtpInstanceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::PtpInstanceNodeStaticModel* getPtpInstanceNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getPtpInstanceNode, notFoundBehavior);
	}
	virtual entity::model::PtpInstanceNodeDynamicModel* getPtpInstanceNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getPtpInstanceNode, notFoundBehavior);
	}

	virtual model::PtpPortNode* getPtpPortNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) = 0;
	virtual entity::model::PtpPortNodeStaticModel* getPtpPortNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeStaticModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getPtpPortNode, notFoundBehavior);
	}
	virtual entity::model::PtpPortNodeDynamicModel* getPtpPortNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior)
	{
		return getNodeDynamicModel(configurationIndex, descriptorIndex, &TreeModelAccessStrategy::getPtpPortNode, notFoundBehavior);
	}

	virtual ~TreeModelAccessStrategy() noexcept = default;

	// Deleted compiler auto-generated methods
	TreeModelAccessStrategy(TreeModelAccessStrategy const&) = delete;
	TreeModelAccessStrategy(TreeModelAccessStrategy&&) = delete;
	TreeModelAccessStrategy& operator=(TreeModelAccessStrategy const&) = delete;
	TreeModelAccessStrategy& operator=(TreeModelAccessStrategy&&) = delete;
};

} // namespace controller
} // namespace avdecc
} // namespace la
