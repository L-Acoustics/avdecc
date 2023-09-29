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
* @file avdeccControllerProxy.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerProxy.hpp"

#include <la/avdecc/utils.hpp>
#include <la/avdecc/executor.hpp>

namespace la
{
namespace avdecc
{
namespace controller
{
ControllerVirtualProxy::ControllerVirtualProxy(protocol::ProtocolInterface const* const protocolInterface, entity::controller::Interface const* const realInterface, entity::controller::Interface const* const virtualInterface) noexcept
	: _protocolInterface{ protocolInterface }
	, _realInterface{ realInterface }
	, _virtualInterface{ virtualInterface }
{
	_executorName = _protocolInterface->getExecutorName();
}

ControllerVirtualProxy::~ControllerVirtualProxy() noexcept
{
	// Flush all pending jobs
	la::avdecc::ExecutorManager::getInstance().flush(_executorName);
}

void ControllerVirtualProxy::setVirtualEntity(UniqueIdentifier const& virtualEntity) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _lock };
	_virtualEntities.insert(virtualEntity);
}

void ControllerVirtualProxy::clearVirtualEntity(UniqueIdentifier const& virtualEntity) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _lock };
	_virtualEntities.erase(virtualEntity);
}

bool ControllerVirtualProxy::isVirtualEntity(UniqueIdentifier const& virtualEntity) const noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _lock };
	return _virtualEntities.find(virtualEntity) != _virtualEntities.end();
}

void ControllerVirtualProxy::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, isPersistent, descriptorType, descriptorIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->acquireEntity(targetEntityID, isPersistent, descriptorType, descriptorIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->acquireEntity(targetEntityID, isPersistent, descriptorType, descriptorIndex, handler);
	}
}

void ControllerVirtualProxy::releaseEntity(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, descriptorType, descriptorIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->releaseEntity(targetEntityID, descriptorType, descriptorIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->releaseEntity(targetEntityID, descriptorType, descriptorIndex, handler);
	}
}

void ControllerVirtualProxy::lockEntity(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, LockEntityHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, descriptorType, descriptorIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->lockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->lockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
	}
}

void ControllerVirtualProxy::unlockEntity(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, UnlockEntityHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, descriptorType, descriptorIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->unlockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->unlockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
	}
}

void ControllerVirtualProxy::queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->queryEntityAvailable(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->queryEntityAvailable(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->queryControllerAvailable(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->queryControllerAvailable(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->registerUnsolicitedNotifications(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->registerUnsolicitedNotifications(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->unregisterUnsolicitedNotifications(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->unregisterUnsolicitedNotifications(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readEntityDescriptor(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readEntityDescriptor(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::readConfigurationDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readConfigurationDescriptor(targetEntityID, configurationIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readConfigurationDescriptor(targetEntityID, configurationIndex, handler);
	}
}

void ControllerVirtualProxy::readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, audioUnitIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readAudioUnitDescriptor(targetEntityID, configurationIndex, audioUnitIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readAudioUnitDescriptor(targetEntityID, configurationIndex, audioUnitIndex, handler);
	}
}

void ControllerVirtualProxy::readStreamInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readStreamInputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readStreamInputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void ControllerVirtualProxy::readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readStreamOutputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readStreamOutputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void ControllerVirtualProxy::readJackInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, jackIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readJackInputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readJackInputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void ControllerVirtualProxy::readJackOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, jackIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readJackOutputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readJackOutputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void ControllerVirtualProxy::readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, avbInterfaceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readAvbInterfaceDescriptor(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readAvbInterfaceDescriptor(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
	}
}

void ControllerVirtualProxy::readClockSourceDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clockSourceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readClockSourceDescriptor(targetEntityID, configurationIndex, clockSourceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readClockSourceDescriptor(targetEntityID, configurationIndex, clockSourceIndex, handler);
	}
}

void ControllerVirtualProxy::readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, memoryObjectIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readMemoryObjectDescriptor(targetEntityID, configurationIndex, memoryObjectIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readMemoryObjectDescriptor(targetEntityID, configurationIndex, memoryObjectIndex, handler);
	}
}

void ControllerVirtualProxy::readLocaleDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, localeIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readLocaleDescriptor(targetEntityID, configurationIndex, localeIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readLocaleDescriptor(targetEntityID, configurationIndex, localeIndex, handler);
	}
}

void ControllerVirtualProxy::readStringsDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, stringsIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readStringsDescriptor(targetEntityID, configurationIndex, stringsIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readStringsDescriptor(targetEntityID, configurationIndex, stringsIndex, handler);
	}
}

void ControllerVirtualProxy::readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readStreamPortInputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readStreamPortInputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
	}
}

void ControllerVirtualProxy::readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readStreamPortOutputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readStreamPortOutputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
	}
}

void ControllerVirtualProxy::readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, externalPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readExternalPortInputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readExternalPortInputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
	}
}

void ControllerVirtualProxy::readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, externalPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readExternalPortOutputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readExternalPortOutputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
	}
}

void ControllerVirtualProxy::readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, internalPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readInternalPortInputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readInternalPortInputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
	}
}

void ControllerVirtualProxy::readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, internalPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readInternalPortOutputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readInternalPortOutputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
	}
}

void ControllerVirtualProxy::readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clusterIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readAudioClusterDescriptor(targetEntityID, configurationIndex, clusterIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readAudioClusterDescriptor(targetEntityID, configurationIndex, clusterIndex, handler);
	}
}

void ControllerVirtualProxy::readAudioMapDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, mapIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readAudioMapDescriptor(targetEntityID, configurationIndex, mapIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readAudioMapDescriptor(targetEntityID, configurationIndex, mapIndex, handler);
	}
}

void ControllerVirtualProxy::readControlDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, ControlDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, controlIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readControlDescriptor(targetEntityID, configurationIndex, controlIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readControlDescriptor(targetEntityID, configurationIndex, controlIndex, handler);
	}
}

void ControllerVirtualProxy::readClockDomainDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clockDomainIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->readClockDomainDescriptor(targetEntityID, configurationIndex, clockDomainIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readClockDomainDescriptor(targetEntityID, configurationIndex, clockDomainIndex, handler);
	}
}

void ControllerVirtualProxy::readTimingDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, TimingDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, timingIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread

				_virtualInterface->readTimingDescriptor(targetEntityID, configurationIndex, timingIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readTimingDescriptor(targetEntityID, configurationIndex, timingIndex, handler);
	}
}

void ControllerVirtualProxy::readPtpInstanceDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, PtpInstanceDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, ptpInstanceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread

				_virtualInterface->readPtpInstanceDescriptor(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readPtpInstanceDescriptor(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
	}
}

void ControllerVirtualProxy::readPtpPortDescriptor(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, PtpPortDescriptorHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, ptpPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread

				_virtualInterface->readPtpPortDescriptor(targetEntityID, configurationIndex, ptpPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->readPtpPortDescriptor(targetEntityID, configurationIndex, ptpPortIndex, handler);
	}
}

void ControllerVirtualProxy::setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setConfiguration(targetEntityID, configurationIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setConfiguration(targetEntityID, configurationIndex, handler);
	}
}

void ControllerVirtualProxy::getConfiguration(UniqueIdentifier const targetEntityID, GetConfigurationHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getConfiguration(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getConfiguration(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, streamFormat, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setStreamInputFormat(targetEntityID, streamIndex, streamFormat, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setStreamInputFormat(targetEntityID, streamIndex, streamFormat, handler);
	}
}

void ControllerVirtualProxy::getStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamInputFormat(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamInputFormat(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, streamFormat, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, handler);
	}
}

void ControllerVirtualProxy::getStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamOutputFormat(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamOutputFormat(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamPortIndex, mapIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamPortInputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamPortInputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
	}
}

void ControllerVirtualProxy::getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamPortIndex, mapIndex, handler]()
			{
				_virtualInterface->getStreamPortOutputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamPortOutputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
	}
}

void ControllerVirtualProxy::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamPortIndex, mappings, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void ControllerVirtualProxy::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamPortIndex, mappings, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void ControllerVirtualProxy::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamPortIndex, mappings, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void ControllerVirtualProxy::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamPortIndex, mappings, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void ControllerVirtualProxy::setStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, info, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setStreamInputInfo(targetEntityID, streamIndex, info, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setStreamInputInfo(targetEntityID, streamIndex, info, handler);
	}
}

void ControllerVirtualProxy::setStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, info, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setStreamOutputInfo(targetEntityID, streamIndex, info, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setStreamOutputInfo(targetEntityID, streamIndex, info, handler);
	}
}

void ControllerVirtualProxy::getStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamInputInfo(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamInputInfo(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::getStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamOutputInfo(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamOutputInfo(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, entityName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setEntityName(targetEntityID, entityName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setEntityName(targetEntityID, entityName, handler);
	}
}

void ControllerVirtualProxy::getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getEntityName(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getEntityName(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, entityGroupName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setEntityGroupName(targetEntityID, entityGroupName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setEntityGroupName(targetEntityID, entityGroupName, handler);
	}
}

void ControllerVirtualProxy::getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getEntityGroupName(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getEntityGroupName(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName, SetConfigurationNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, configurationName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setConfigurationName(targetEntityID, configurationIndex, configurationName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setConfigurationName(targetEntityID, configurationIndex, configurationName, handler);
	}
}

void ControllerVirtualProxy::getConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getConfigurationName(targetEntityID, configurationIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getConfigurationName(targetEntityID, configurationIndex, handler);
	}
}

void ControllerVirtualProxy::setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, audioUnitIndex, audioUnitName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, audioUnitName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, audioUnitName, handler);
	}
}

void ControllerVirtualProxy::getAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, audioUnitIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, handler);
	}
}

void ControllerVirtualProxy::setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamIndex, streamInputName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setStreamInputName(targetEntityID, configurationIndex, streamIndex, streamInputName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setStreamInputName(targetEntityID, configurationIndex, streamIndex, streamInputName, handler);
	}
}

void ControllerVirtualProxy::getStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamInputName(targetEntityID, configurationIndex, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamInputName(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void ControllerVirtualProxy::setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamIndex, streamOutputName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setStreamOutputName(targetEntityID, configurationIndex, streamIndex, streamOutputName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setStreamOutputName(targetEntityID, configurationIndex, streamIndex, streamOutputName, handler);
	}
}

void ControllerVirtualProxy::getStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamOutputName(targetEntityID, configurationIndex, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamOutputName(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void ControllerVirtualProxy::setJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName, SetJackInputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, jackIndex, jackInputName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setJackInputName(targetEntityID, configurationIndex, jackIndex, jackInputName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setJackInputName(targetEntityID, configurationIndex, jackIndex, jackInputName, handler);
	}
}

void ControllerVirtualProxy::getJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, GetJackInputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, jackIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getJackInputName(targetEntityID, configurationIndex, jackIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getJackInputName(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void ControllerVirtualProxy::setJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName, SetJackOutputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, jackIndex, jackOutputName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setJackOutputName(targetEntityID, configurationIndex, jackIndex, jackOutputName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setJackOutputName(targetEntityID, configurationIndex, jackIndex, jackOutputName, handler);
	}
}

void ControllerVirtualProxy::getJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, GetJackOutputNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, jackIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getJackOutputName(targetEntityID, configurationIndex, jackIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getJackOutputName(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void ControllerVirtualProxy::setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, avbInterfaceIndex, avbInterfaceName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, avbInterfaceName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, avbInterfaceName, handler);
	}
}

void ControllerVirtualProxy::getAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, avbInterfaceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
	}
}

void ControllerVirtualProxy::setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clockSourceIndex, clockSourceName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, clockSourceName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, clockSourceName, handler);
	}
}

void ControllerVirtualProxy::getClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clockSourceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, handler);
	}
}

void ControllerVirtualProxy::setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, memoryObjectIndex, memoryObjectName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, memoryObjectName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, memoryObjectName, handler);
	}
}

void ControllerVirtualProxy::getMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, memoryObjectIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, handler);
	}
}

void ControllerVirtualProxy::setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, audioClusterIndex, audioClusterName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, audioClusterName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, audioClusterName, handler);
	}
}

void ControllerVirtualProxy::getAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, audioClusterIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, handler);
	}
}

void ControllerVirtualProxy::setControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName, SetControlNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, controlIndex, controlName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setControlName(targetEntityID, configurationIndex, controlIndex, controlName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setControlName(targetEntityID, configurationIndex, controlIndex, controlName, handler);
	}
}

void ControllerVirtualProxy::getControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, GetControlNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, controlIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getControlName(targetEntityID, configurationIndex, controlIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getControlName(targetEntityID, configurationIndex, controlIndex, handler);
	}
}

void ControllerVirtualProxy::setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clockDomainIndex, clockDomainName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, clockDomainName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, clockDomainName, handler);
	}
}

void ControllerVirtualProxy::getClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, clockDomainIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, handler);
	}
}

void ControllerVirtualProxy::setTimingName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& timingName, SetTimingNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, timingIndex, timingName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setTimingName(targetEntityID, configurationIndex, timingIndex, timingName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setTimingName(targetEntityID, configurationIndex, timingIndex, timingName, handler);
	}
}

void ControllerVirtualProxy::getTimingName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, GetTimingNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, timingIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getTimingName(targetEntityID, configurationIndex, timingIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getTimingName(targetEntityID, configurationIndex, timingIndex, handler);
	}
}

void ControllerVirtualProxy::setPtpInstanceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& ptpInstanceName, SetPtpInstanceNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, ptpInstanceIndex, ptpInstanceName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, ptpInstanceName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, ptpInstanceName, handler);
	}
}

void ControllerVirtualProxy::getPtpInstanceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, GetPtpInstanceNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, ptpInstanceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
	}
}

void ControllerVirtualProxy::setPtpPortName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& ptpPortName, SetPtpPortNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, ptpPortIndex, ptpPortName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, ptpPortName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, ptpPortName, handler);
	}
}

void ControllerVirtualProxy::getPtpPortName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, GetPtpPortNameHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, ptpPortIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, handler);
	}
}

void ControllerVirtualProxy::setAssociation(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, associationID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setAssociation(targetEntityID, associationID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setAssociation(targetEntityID, associationID, handler);
	}
}

void ControllerVirtualProxy::getAssociation(UniqueIdentifier const targetEntityID, GetAssociationHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAssociation(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAssociation(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, audioUnitIndex, samplingRate, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate, handler);
	}
}

void ControllerVirtualProxy::getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, audioUnitIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAudioUnitSamplingRate(targetEntityID, audioUnitIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAudioUnitSamplingRate(targetEntityID, audioUnitIndex, handler);
	}
}

void ControllerVirtualProxy::setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const videoClusterIndex, entity::model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, videoClusterIndex, samplingRate, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setVideoClusterSamplingRate(targetEntityID, videoClusterIndex, samplingRate, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setVideoClusterSamplingRate(targetEntityID, videoClusterIndex, samplingRate, handler);
	}
}

void ControllerVirtualProxy::getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, videoClusterIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getVideoClusterSamplingRate(targetEntityID, videoClusterIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getVideoClusterSamplingRate(targetEntityID, videoClusterIndex, handler);
	}
}

void ControllerVirtualProxy::setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const sensorClusterIndex, entity::model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, sensorClusterIndex, samplingRate, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, samplingRate, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, samplingRate, handler);
	}
}

void ControllerVirtualProxy::getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, entity::model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, sensorClusterIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, handler);
	}
}

void ControllerVirtualProxy::setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, clockDomainIndex, clockSourceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex, handler);
	}
}

void ControllerVirtualProxy::getClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, clockDomainIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getClockSource(targetEntityID, clockDomainIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getClockSource(targetEntityID, clockDomainIndex, handler);
	}
}

void ControllerVirtualProxy::setControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, controlIndex, controlValues, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setControlValues(targetEntityID, controlIndex, controlValues, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setControlValues(targetEntityID, controlIndex, controlValues, handler);
	}
}

void ControllerVirtualProxy::getControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, GetControlValuesHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, controlIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getControlValues(targetEntityID, controlIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getControlValues(targetEntityID, controlIndex, handler);
	}
}

void ControllerVirtualProxy::startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->startStreamInput(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->startStreamInput(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->startStreamOutput(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->startStreamOutput(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->stopStreamInput(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->stopStreamInput(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->stopStreamOutput(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->stopStreamOutput(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::getAvbInfo(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, avbInterfaceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAvbInfo(targetEntityID, avbInterfaceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAvbInfo(targetEntityID, avbInterfaceIndex, handler);
	}
}

void ControllerVirtualProxy::getAsPath(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAsPathHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, avbInterfaceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAsPath(targetEntityID, avbInterfaceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAsPath(targetEntityID, avbInterfaceIndex, handler);
	}
}

void ControllerVirtualProxy::getEntityCounters(UniqueIdentifier const targetEntityID, GetEntityCountersHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getEntityCounters(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getEntityCounters(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, avbInterfaceIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getAvbInterfaceCounters(targetEntityID, avbInterfaceIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getAvbInterfaceCounters(targetEntityID, avbInterfaceIndex, handler);
	}
}

void ControllerVirtualProxy::getClockDomainCounters(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, clockDomainIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getClockDomainCounters(targetEntityID, clockDomainIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getClockDomainCounters(targetEntityID, clockDomainIndex, handler);
	}
}

void ControllerVirtualProxy::getStreamInputCounters(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamInputCounters(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamInputCounters(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::getStreamOutputCounters(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamOutputCountersHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamOutputCounters(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getStreamOutputCounters(targetEntityID, streamIndex, handler);
	}
}

void ControllerVirtualProxy::reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->reboot(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->reboot(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::rebootToFirmware(UniqueIdentifier const targetEntityID, entity::model::MemoryObjectIndex const memoryObjectIndex, RebootToFirmwareHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, memoryObjectIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->rebootToFirmware(targetEntityID, memoryObjectIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->rebootToFirmware(targetEntityID, memoryObjectIndex, handler);
	}
}

void ControllerVirtualProxy::startOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, descriptorType, descriptorIndex, operationType, memoryBuffer, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->startOperation(targetEntityID, descriptorType, descriptorIndex, operationType, memoryBuffer, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->startOperation(targetEntityID, descriptorType, descriptorIndex, operationType, memoryBuffer, handler);
	}
}

void ControllerVirtualProxy::abortOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, descriptorType, descriptorIndex, operationID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->abortOperation(targetEntityID, descriptorType, descriptorIndex, operationID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->abortOperation(targetEntityID, descriptorType, descriptorIndex, operationID, handler);
	}
}

void ControllerVirtualProxy::setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, memoryObjectIndex, length, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, length, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->setMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, length, handler);
	}
}

void ControllerVirtualProxy::getMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, configurationIndex, memoryObjectIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, handler);
	}
}

void ControllerVirtualProxy::addressAccess(UniqueIdentifier const targetEntityID, entity::addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, tlvs, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->addressAccess(targetEntityID, tlvs, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->addressAccess(targetEntityID, tlvs, handler);
	}
}

void ControllerVirtualProxy::getMilanInfo(UniqueIdentifier const targetEntityID, GetMilanInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getMilanInfo(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getMilanInfo(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(listenerStream.entityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, talkerStream, listenerStream, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->connectStream(talkerStream, listenerStream, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->connectStream(talkerStream, listenerStream, handler);
	}
}

void ControllerVirtualProxy::disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(listenerStream.entityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, talkerStream, listenerStream, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->disconnectStream(talkerStream, listenerStream, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->disconnectStream(talkerStream, listenerStream, handler);
	}
}

void ControllerVirtualProxy::disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(talkerStream.entityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, talkerStream, listenerStream, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->disconnectTalkerStream(talkerStream, listenerStream, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->disconnectTalkerStream(talkerStream, listenerStream, handler);
	}
}

void ControllerVirtualProxy::getTalkerStreamState(entity::model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(talkerStream.entityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, talkerStream, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getTalkerStreamState(talkerStream, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getTalkerStreamState(talkerStream, handler);
	}
}

void ControllerVirtualProxy::getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(listenerStream.entityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, listenerStream, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getListenerStreamState(listenerStream, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getListenerStreamState(listenerStream, handler);
	}
}

void ControllerVirtualProxy::getTalkerStreamConnection(entity::model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(talkerStream.entityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, talkerStream, connectionIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getTalkerStreamConnection(talkerStream, connectionIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		_realInterface->getTalkerStreamConnection(talkerStream, connectionIndex, handler);
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
