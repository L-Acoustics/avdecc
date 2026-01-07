/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file avdeccVirtualControlledEntityInterface.hpp
* @author Christophe Calmejane
* @brief Proxy class to change read-only values of a virtual #la::avdecc::controller::ControlledEntity.
*/

#pragma once

#include <la/avdecc/controller/internals/avdeccControlledEntity.hpp>


namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************************** */
/*                                                                            */
/* ************************************************************************** */
/**
* @brief Interface to set read-only values of a virtual ControlledEntity.
* @details This interface is used to set read-only values of a virtual ControlledEntity. If any of the methods is called on a non-virtual ControlledEntity, the call will be silently ignored.
*/
class VirtualControlledEntityInterface
{
public:
	virtual ~VirtualControlledEntityInterface() noexcept = default;
	virtual void setAcquiredState(UniqueIdentifier const targetEntityID, model::AcquireState const acquireState, UniqueIdentifier const owningEntity) noexcept = 0;
	virtual void setEntityLockedState(UniqueIdentifier const targetEntityID, model::LockState const lockState, UniqueIdentifier const lockingEntity) noexcept = 0;
	virtual void setEntityCounters(UniqueIdentifier const targetEntityID, entity::model::EntityCounters const& counters) noexcept = 0;
	virtual void setAvbInterfaceCounters(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInterfaceCounters const& counters) noexcept = 0;
	virtual void setClockDomainCounters(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainCounters const& counters) noexcept = 0;
	virtual void setStreamInputCounters(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInputCounters const& counters) noexcept = 0;
	virtual void setStreamOutputCounters(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamOutputCounters const& counters) noexcept = 0;
};

} // namespace controller
} // namespace avdecc
} // namespace la
