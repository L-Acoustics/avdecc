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
* @file avdeccEntityModelCache.hpp
* @author Christophe Calmejane
*/

#pragma once

#include <la/avdecc/internals/entityModelTree.hpp>

#include <unordered_map>

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

	void enableCache() noexcept
	{
		_isEnabled = true;
	}

	void disableCache() noexcept
	{
		_isEnabled = false;
	}

	// TODO: If we want to add a clearCache method, we'll have to add locking to this class
	// because clearCache would probably not be called from the same thread than getCachedEntityTree and cacheEntityTree.
	// Also we should return a copy of the data (pair<bool, Tree> or std::optional) so the lock is release with valid data
	entity::model::EntityTree const* getCachedEntityTree(UniqueIdentifier const entityModelID, entity::model::ConfigurationIndex const configurationIndex) const noexcept
	{
		if (_isEnabled && entityModelID)
		{
			auto const entityModelIt = _modelCache.find(entityModelID);
			if (entityModelIt != _modelCache.end())
			{
				auto const& entityModel = entityModelIt->second;
				auto const modelIt = entityModel.find(configurationIndex);
				if (modelIt != entityModel.end())
				{
					return &modelIt->second;
				}
			}
		}

		return nullptr;
	}

	void cacheEntityTree(UniqueIdentifier const entityModelID, entity::model::ConfigurationIndex const configurationIndex, entity::model::EntityTree const& tree) noexcept
	{
		if (_isEnabled && entityModelID)
		{
			auto& entityModel = _modelCache[entityModelID];

			// Cache the EntityModel but only if not already in cache
			auto modelIt = entityModel.find(configurationIndex);
			if (modelIt == entityModel.end())
			{
				// Make a copy of the tree
				auto cachedTree = tree;

				// Wipe all the dynamic model
				cachedTree.dynamicModel = {};
				for (auto& configKV : cachedTree.configurationTrees)
				{
					auto& config = configKV.second;
					config.dynamicModel = {};
					for (auto& KV : config.audioUnitModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.streamInputModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.streamOutputModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.avbInterfaceModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.clockSourceModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.memoryObjectModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.streamPortInputModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.streamPortOutputModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.audioClusterModels)
					{
						KV.second.dynamicModel = {};
					}
					for (auto& KV : config.clockDomainModels)
					{
						KV.second.dynamicModel = {};
					}
				}

				// Move it to the cache
				entityModel.insert(std::make_pair(configurationIndex, std::move(cachedTree)));
			}
		}
	}

private:
	using ConfigurationModels = std::unordered_map<entity::model::ConfigurationIndex, entity::model::EntityTree>;
	std::unordered_map<UniqueIdentifier, ConfigurationModels, la::avdecc::UniqueIdentifier::hash> _modelCache{};
	bool _isEnabled{ false };
};

} // namespace controller
} // namespace avdecc
} // namespace la
