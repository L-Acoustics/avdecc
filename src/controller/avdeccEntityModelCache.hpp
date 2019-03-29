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

#include "avdeccControlledEntityModelTree.hpp"

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
	// because clearCache would probably not be called from the same thread than getCachedEntityStaticTree and cacheEntityStaticTree.
	// Also we should return a copy of the data (pair<bool, Tree> or std::optional) so the lock is release with valid data

	model::EntityStaticTree const* getCachedEntityStaticTree(UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) const noexcept
	{
		if (_isEnabled)
		{
			auto const entityModelIt = _modelCache.find(entityID);
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

	void cacheEntityStaticTree(UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, model::EntityStaticTree const& staticTree) noexcept
	{
		if (_isEnabled)
		{
			auto& entityModel = _modelCache[entityID];

			// Cache the EntityModel but only if not already in cache
			auto modelIt = entityModel.find(configurationIndex);
			if (modelIt == entityModel.end())
			{
				entityModel.insert(std::make_pair(configurationIndex, staticTree));
			}
		}
	}

private:
	using StaticEntityModel = std::unordered_map<entity::model::ConfigurationIndex, model::EntityStaticTree>;
	std::unordered_map<UniqueIdentifier, StaticEntityModel, la::avdecc::UniqueIdentifier::hash> _modelCache{};
	bool _isEnabled{ false };
};

} // namespace controller
} // namespace avdecc
} // namespace la
