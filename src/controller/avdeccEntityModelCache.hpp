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
* @file avdeccEntityModelCache.hpp
* @author Christophe Calmejane
*/

#pragma once

#include <la/avdecc/controller/internals/avdeccControlledEntityModel.hpp>

#include "avdeccControllerLogHelper.hpp"

#include <unordered_map>
#include <mutex>
#include <optional>

namespace la
{
namespace avdecc
{
namespace controller
{
class EntityModelCache final
{
public:
	static EntityModelCache& getInstance() noexcept
	{
		static EntityModelCache s_instance{};
		return s_instance;
	}

	bool isCacheEnabled() const noexcept
	{
		auto const lg = std::lock_guard{ _lock };

		return _isEnabled;
	}

	void enableCache() noexcept
	{
		auto const lg = std::lock_guard{ _lock };

		_isEnabled = true;
	}

	void disableCache() noexcept
	{
		auto const lg = std::lock_guard{ _lock };

		_isEnabled = false;
	}

	std::optional<model::EntityNode> getCachedEntityModel(UniqueIdentifier const entityModelID) const noexcept
	{
		AVDECC_ASSERT(_isEnabled, "Should not call AEM cache if cache is not enabled");
		AVDECC_ASSERT(entityModelID, "Should not call AEM cache if EntityModelID is invalid");
		auto const lg = std::lock_guard{ _lock };

		if (_isEnabled && entityModelID)
		{
			if (auto const entityModelIt = _modelCache.find(entityModelID); entityModelIt != _modelCache.end())
			{
				return entityModelIt->second;
			}
		}

		return std::nullopt;
	}

	void cacheEntityModel(UniqueIdentifier const entityModelID, model::EntityNode&& model) noexcept
	{
		AVDECC_ASSERT(_isEnabled, "Should not call AEM cache if cache is not enabled");
		AVDECC_ASSERT(entityModelID, "Should not call AEM cache if EntityModelID is invalid");
		auto const lg = std::lock_guard{ _lock };

		if (_isEnabled && entityModelID)
		{
			// Cache the EntityModel but only if not already in cache
			if (_modelCache.count(entityModelID) == 0)
			{
				// Move it to the cache
				_modelCache.emplace(entityModelID, std::move(model));
			}
		}
	}

	static inline bool isValidEntityModelID(UniqueIdentifier const entityModelID) noexcept
	{
		auto const [vendorID, deviceID, modelID] = entity::model::splitEntityModelID(entityModelID);
		return vendorID != 0x00000000 && vendorID != 0x00FFFFFF;
	}

	static inline bool isModelValidForConfiguration(model::ConfigurationNode const& configNode) noexcept
	{
		// Check TOP LEVEL descriptors count. If the declared count does not match what is stored in the tree, it probably means we didn't have a valid tree for this configuration (model was only partially stored)
		// Currently, we don't want to check more deeply as we trust both the AEM loader and the enumeration state machine to give us a valid model
		auto const& descriptorCounts = configNode.staticModel.descriptorCounts;
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::AudioUnit, configNode.audioUnits))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::StreamInput, configNode.streamInputs))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::StreamOutput, configNode.streamOutputs))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::JackInput, configNode.jackInputs))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::JackOutput, configNode.jackOutputs))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::AvbInterface, configNode.avbInterfaces))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::ClockSource, configNode.clockSources))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::MemoryObject, configNode.memoryObjects))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::Control, configNode.controls))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::ClockDomain, configNode.clockDomains))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::Timing, configNode.timings))
		{
			return false;
		}
		if (!validateDescriptorCount(descriptorCounts, entity::model::DescriptorType::PtpInstance, configNode.ptpInstances))
		{
			return false;
		}

		// Seems valid
		return true;
	}

private:
	template<class Tree>
	static inline bool validateDescriptorCount(std::unordered_map<entity::model::DescriptorType, std::uint16_t, la::avdecc::utils::EnumClassHash> const& descriptorCounts, entity::model::DescriptorType const descriptorType, Tree const& tree) noexcept
	{
		auto count = std::uint16_t{ 0u };
		if (auto const descIt = descriptorCounts.find(descriptorType); descIt != descriptorCounts.end())
		{
			count = descIt->second;
		}
		return tree.size() == count;
	}

	mutable std::mutex _lock{};
	std::unordered_map<UniqueIdentifier, model::EntityNode, la::avdecc::UniqueIdentifier::hash> _modelCache{};
	bool _isEnabled{ false };
};

} // namespace controller
} // namespace avdecc
} // namespace la
