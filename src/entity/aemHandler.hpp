/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file aemHandler.hpp
* @author Christophe Calmejane
*/

#pragma once

//#include "la/avdecc/internals/controllerEntity.hpp"

//#include "entityImpl.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
class AemHandler final
{
public:
	AemHandler(entity::Entity const& entity, entity::model::EntityTree const* const entityModelTree) noexcept;

	bool onUnhandledAecpAemCommand(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aem) const noexcept;

	// Deleted compiler auto-generated methods
	AemHandler(AemHandler const&) = delete;
	AemHandler(AemHandler&&) = delete;
	AemHandler& operator=(AemHandler const&) = delete;
	AemHandler& operator=(AemHandler&&) = delete;

private:
	EntityDescriptor buildEntityDescriptor() const noexcept;

	entity::Entity const& _entity;
	entity::model::EntityTree const* _entityModelTree{ nullptr };
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
