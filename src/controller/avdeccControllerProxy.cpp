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
* @file avdeccControllerProxy.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerProxy.hpp"

#include <la/avdecc/utils.hpp>
#include <la/avdecc/executor.hpp>

#include <memory>
#include <tuple>
#include <type_traits>

namespace la
{
namespace avdecc
{
namespace controller
{
namespace
{
/**
* @brief Builds a retrying handler wrapper for AEM/AA/MVU commands (status is the 3rd parameter).
* @details Wraps the user-provided handler so that, on a retryable error (TimedOut/UnknownEntity/NetworkError), the command is re-sent on the other physical interface (if any) before invoking the original handler. The retry is attempted at most once.
* @param[in] proxy The owning proxy (used to query reachability and retry policy).
* @param[in] originalHandler The user-provided completion handler.
* @param[in] retryInvoker Callable invoked to resend the command on the other interface, given that interface and the (same) handler to use for the retry attempt.
* @return A new handler with the same signature as @a originalHandler.
*/
template<typename Handler, typename Invoker>
Handler makeRetryAemHandler(ControllerVirtualProxy const* const proxy, Handler const& originalHandler, Invoker const& retryInvoker) noexcept
{
	auto retried = std::make_shared<bool>(false);
	return Handler{ [proxy, originalHandler, retryInvoker, retried](entity::controller::Interface const* const sender, UniqueIdentifier const eid, auto const status, auto const&... rest) noexcept
		{
			if (!*retried && proxy->isDualInterface() && ControllerVirtualProxy::shouldRetry(status))
			{
				// Only retry if the entity is currently reachable on the other PI. This avoids converting a legitimate
				// failure (e.g. a real TimedOut from the only PI that ever saw the entity) into a misleading UnknownEntity
				// returned by a PI that never discovered the target.
				auto* const other = proxy->otherReachableInterface(eid, sender);
				if (other != nullptr)
				{
					*retried = true;
					retryInvoker(other, originalHandler);
					return;
				}
			}
			if (originalHandler)
			{
				originalHandler(sender, eid, status, rest...);
			}
		} };
}

/**
* @brief Builds a retrying handler wrapper for ACMP commands (status is the LAST parameter).
* @details Same intent as makeRetryAemHandler but for ACMP completion handlers, where the ControlStatus is the final argument.
* @param[in] proxy The owning proxy.
* @param[in] originalHandler The user-provided completion handler.
* @param[in] retryInvoker Callable invoked to resend the command on the other interface.
* @return A new handler with the same signature as @a originalHandler.
*/
template<typename Handler, typename Invoker>
Handler makeRetryAcmpHandler(ControllerVirtualProxy const* const proxy, UniqueIdentifier const routingEntityID, Handler const& originalHandler, Invoker const& retryInvoker) noexcept
{
	auto retried = std::make_shared<bool>(false);
	return Handler{ [proxy, routingEntityID, originalHandler, retryInvoker, retried](entity::controller::Interface const* const sender, auto const&... args) noexcept
		{
			static_assert(sizeof...(args) >= 1u, "ACMP handler must have at least one argument (the ControlStatus)");
			auto const argsTuple = std::forward_as_tuple(args...);
			auto const& status = std::get<sizeof...(args) - 1u>(argsTuple);
			if (!*retried && proxy->isDualInterface() && ControllerVirtualProxy::shouldRetry(status))
			{
				auto* const other = proxy->otherReachableInterface(routingEntityID, sender);
				if (other != nullptr)
				{
					*retried = true;
					retryInvoker(other, originalHandler);
					return;
				}
			}
			if (originalHandler)
			{
				originalHandler(sender, args...);
			}
		} };
}
} // namespace

/**
* @brief Routes an AEM/AA/MVU forwarding call through the dual-interface retry layer.
* @details Selects the appropriate real interface using @ref pickRealInterface and wraps the handler so that retryable errors automatically re-issue the command on the other physical interface.
*/
template<auto Method, typename HandlerT, typename... Args>
void ControllerVirtualProxy::routeAemCommand(UniqueIdentifier const targetEntityID, HandlerT const& handler, Args const&... args) const noexcept
{
	auto invoker = std::function<void(entity::controller::Interface const* const, HandlerT const&)>{ [targetEntityID, args...](entity::controller::Interface const* const iface, HandlerT const& h) noexcept
		{
			(iface->*Method)(targetEntityID, args..., h);
		} };
	auto wrapped = makeRetryAemHandler(this, handler, invoker);
	(pickRealInterface(targetEntityID)->*Method)(targetEntityID, args..., wrapped);
}

/**
* @brief Routes an ACMP forwarding call through the dual-interface retry layer (status is the last parameter).
*/
template<auto Method, typename HandlerT, typename... Args>
void ControllerVirtualProxy::routeAcmpCommand(UniqueIdentifier const routingKey, HandlerT const& handler, Args const&... args) const noexcept
{
	auto invoker = std::function<void(entity::controller::Interface const* const, HandlerT const&)>{ [args...](entity::controller::Interface const* const iface, HandlerT const& h) noexcept
		{
			(iface->*Method)(args..., h);
		} };
	auto wrapped = makeRetryAcmpHandler(this, routingKey, handler, invoker);
	(pickRealInterface(routingKey)->*Method)(args..., wrapped);
}

ControllerVirtualProxy::ControllerVirtualProxy(protocol::ProtocolInterface const* const protocolInterface, entity::controller::Interface const* const realInterface, entity::controller::Interface const* const virtualInterface) noexcept
	: _protocolInterface{ protocolInterface }
	, _realInterface{ realInterface }
	, _secondaryRealInterface{ nullptr }
	, _virtualInterface{ virtualInterface }
{
	_executorName = _protocolInterface->getExecutorName();
}

ControllerVirtualProxy::ControllerVirtualProxy(protocol::ProtocolInterface const* const protocolInterface, entity::controller::Interface const* const primaryRealInterface, entity::controller::Interface const* const secondaryRealInterface, entity::controller::Interface const* const virtualInterface) noexcept
	: _protocolInterface{ protocolInterface }
	, _realInterface{ primaryRealInterface }
	, _secondaryRealInterface{ secondaryRealInterface }
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

bool ControllerVirtualProxy::setEntityReachable(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType, bool const reachable) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto& info = _reachability[entityID];
	auto const wasReachable = (interfaceType == Controller::InterfaceType::Primary) ? info.onPrimary : info.onSecondary;
	if (interfaceType == Controller::InterfaceType::Primary)
	{
		info.onPrimary = reachable;
		if (!reachable)
		{
			// PI lost reachability: drop any unsol registration we believed was active here, so the next time we see the entity on this PI we re-register.
			info.unsolPrimary = UnsolState::NotRegistered;
		}
	}
	else
	{
		info.onSecondary = reachable;
		if (!reachable)
		{
			info.unsolSecondary = UnsolState::NotRegistered;
		}
	}
	return reachable && !wasReachable;
}

void ControllerVirtualProxy::clearEntityReachability(UniqueIdentifier const& entityID) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	_reachability.erase(entityID);
}

bool ControllerVirtualProxy::markInterfaceDown(Controller::InterfaceType const interfaceType) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };

	// Update the per-PI transport-up state first (independent of any discovered entity).
	if (interfaceType == Controller::InterfaceType::Primary)
	{
		_primaryInterfaceUp = false;
	}
	else
	{
		_secondaryInterfaceUp = false;
	}

	// Also clear per-entity reachability flags for the failing PI so subsequent routing decisions are accurate.
	for (auto& [entityID, info] : _reachability)
	{
		if (interfaceType == Controller::InterfaceType::Primary)
		{
			info.onPrimary = false;
			info.unsolPrimary = UnsolState::NotRegistered;
		}
		else
		{
			info.onSecondary = false;
			info.unsolSecondary = UnsolState::NotRegistered;
		}
	}

	// The redundancy guarantee is at the transport level: the controller is still operational as long as the
	// other PI's transport has not also collapsed. This must NOT be conditioned on having already discovered
	// entities (which would incorrectly escalate to a fatal error on early-boot transport faults).
	return (interfaceType == Controller::InterfaceType::Primary) ? _secondaryInterfaceUp : _primaryInterfaceUp;
}

ControllerVirtualProxy::InterfaceReachability ControllerVirtualProxy::getEntityReachability(UniqueIdentifier const& entityID) const noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto const it = _reachability.find(entityID);
	if (it == _reachability.end())
	{
		return InterfaceReachability{};
	}
	return InterfaceReachability{ it->second.onPrimary, it->second.onSecondary };
}

bool ControllerVirtualProxy::isDualInterface() const noexcept
{
	return _secondaryRealInterface != nullptr;
}

entity::controller::Interface const* ControllerVirtualProxy::getSecondaryRealInterface() const noexcept
{
	return _secondaryRealInterface;
}

entity::controller::Interface const* ControllerVirtualProxy::pickRealInterface(UniqueIdentifier const& targetEntityID) const noexcept
{
	// Single-interface mode: always primary
	if (_secondaryRealInterface == nullptr)
	{
		return _realInterface;
	}

	// Dual-interface mode: pick based on reachability
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto const it = _reachability.find(targetEntityID);
	if (it == _reachability.end())
	{
		// Unknown entity: default to primary
		return _realInterface;
	}

	auto const& info = it->second;
	// Prefer primary when available
	if (info.onPrimary)
	{
		return _realInterface;
	}
	if (info.onSecondary)
	{
		return _secondaryRealInterface;
	}
	// Neither interface reports the entity as reachable: default to primary so the command at least exits the controller (it will likely fail and that's OK)
	return _realInterface;
}

entity::controller::Interface const* ControllerVirtualProxy::otherRealInterface(entity::controller::Interface const* const chosenInterface) const noexcept
{
	if (_secondaryRealInterface == nullptr)
	{
		return nullptr;
	}
	if (chosenInterface == _realInterface)
	{
		return _secondaryRealInterface;
	}
	return _realInterface;
}

entity::controller::Interface const* ControllerVirtualProxy::otherReachableInterface(UniqueIdentifier const& entityID, entity::controller::Interface const* const chosenInterface) const noexcept
{
	auto* const other = otherRealInterface(chosenInterface);
	if (other == nullptr)
	{
		return nullptr;
	}
	// Look up the reachability for the other PI: only allow retry there if the entity has actually been seen on it.
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto const it = _reachability.find(entityID);
	if (it == _reachability.end())
	{
		return nullptr;
	}
	auto const& info = it->second;
	auto const otherIsPrimary = (other == _realInterface);
	if (otherIsPrimary && info.onPrimary)
	{
		return other;
	}
	if (!otherIsPrimary && info.onSecondary)
	{
		return other;
	}
	return nullptr;
}

bool ControllerVirtualProxy::tryClaimUnsolPending(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto const it = _reachability.find(entityID);
	if (it == _reachability.end())
	{
		// We don't track unsol for entities that we haven't seen on any PI yet.
		return false;
	}
	auto& info = it->second;
	auto& state = (interfaceType == Controller::InterfaceType::Primary) ? info.unsolPrimary : info.unsolSecondary;
	if (state != UnsolState::NotRegistered)
	{
		return false;
	}
	state = UnsolState::Pending;
	return true;
}

void ControllerVirtualProxy::setUnsolState(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType, UnsolState const newState) noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto const it = _reachability.find(entityID);
	if (it == _reachability.end())
	{
		return;
	}
	auto& info = it->second;
	if (interfaceType == Controller::InterfaceType::Primary)
	{
		info.unsolPrimary = newState;
	}
	else
	{
		info.unsolSecondary = newState;
	}
}

ControllerVirtualProxy::UnsolState ControllerVirtualProxy::getUnsolState(UniqueIdentifier const& entityID, Controller::InterfaceType const interfaceType) const noexcept
{
	auto const lg = std::lock_guard<std::mutex>{ _reachabilityLock };
	auto const it = _reachability.find(entityID);
	if (it == _reachability.end())
	{
		return UnsolState::NotRegistered;
	}
	auto const& info = it->second;
	return (interfaceType == Controller::InterfaceType::Primary) ? info.unsolPrimary : info.unsolSecondary;
}

void ControllerVirtualProxy::registerUnsolicitedNotificationsOnInterface(UniqueIdentifier const targetEntityID, Controller::InterfaceType const interfaceType, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	// Pick the concrete real PI to use. In single-PI mode the secondary interface is null and we always use the primary.
	auto const* const targetInterface = (interfaceType == Controller::InterfaceType::Secondary && _secondaryRealInterface != nullptr) ? _secondaryRealInterface : _realInterface;
	if (targetInterface == nullptr)
	{
		return;
	}
	// No dual-PI retry wrapping here: this method is used by the lazy per-PI registration logic and must hit exactly one PI.
	targetInterface->registerUnsolicitedNotifications(targetEntityID, handler);
}

bool ControllerVirtualProxy::shouldRetry(entity::ControllerEntity::AemCommandStatus const status) noexcept
{
	switch (status)
	{
		case entity::ControllerEntity::AemCommandStatus::TimedOut:
		case entity::ControllerEntity::AemCommandStatus::UnknownEntity:
		case entity::ControllerEntity::AemCommandStatus::NetworkError:
			return true;
		default:
			return false;
	}
}

bool ControllerVirtualProxy::shouldRetry(entity::ControllerEntity::AaCommandStatus const status) noexcept
{
	switch (status)
	{
		case entity::ControllerEntity::AaCommandStatus::TimedOut:
		case entity::ControllerEntity::AaCommandStatus::UnknownEntity:
		case entity::ControllerEntity::AaCommandStatus::NetworkError:
			return true;
		default:
			return false;
	}
}

bool ControllerVirtualProxy::shouldRetry(entity::ControllerEntity::MvuCommandStatus const status) noexcept
{
	switch (status)
	{
		case entity::ControllerEntity::MvuCommandStatus::TimedOut:
		case entity::ControllerEntity::MvuCommandStatus::UnknownEntity:
		case entity::ControllerEntity::MvuCommandStatus::NetworkError:
			return true;
		default:
			return false;
	}
}

bool ControllerVirtualProxy::shouldRetry(entity::ControllerEntity::ControlStatus const status) noexcept
{
	switch (status)
	{
		case entity::ControllerEntity::ControlStatus::TimedOut:
		case entity::ControllerEntity::ControlStatus::ListenerUnknownID:
		case entity::ControllerEntity::ControlStatus::TalkerUnknownID:
		case entity::ControllerEntity::ControlStatus::NetworkError:
			return true;
		default:
			return false;
	}
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
		routeAemCommand<&entity::controller::Interface::acquireEntity>(targetEntityID, handler, isPersistent, descriptorType, descriptorIndex);
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
		routeAemCommand<&entity::controller::Interface::releaseEntity>(targetEntityID, handler, descriptorType, descriptorIndex);
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
		routeAemCommand<&entity::controller::Interface::lockEntity>(targetEntityID, handler, descriptorType, descriptorIndex);
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
		routeAemCommand<&entity::controller::Interface::unlockEntity>(targetEntityID, handler, descriptorType, descriptorIndex);
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
		routeAemCommand<&entity::controller::Interface::queryEntityAvailable>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::queryControllerAvailable>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::registerUnsolicitedNotifications>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::unregisterUnsolicitedNotifications>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::readEntityDescriptor>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::readConfigurationDescriptor>(targetEntityID, handler, configurationIndex);
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
		routeAemCommand<&entity::controller::Interface::readAudioUnitDescriptor>(targetEntityID, handler, configurationIndex, audioUnitIndex);
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
		routeAemCommand<&entity::controller::Interface::readStreamInputDescriptor>(targetEntityID, handler, configurationIndex, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::readStreamOutputDescriptor>(targetEntityID, handler, configurationIndex, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::readJackInputDescriptor>(targetEntityID, handler, configurationIndex, jackIndex);
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
		routeAemCommand<&entity::controller::Interface::readJackOutputDescriptor>(targetEntityID, handler, configurationIndex, jackIndex);
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
		routeAemCommand<&entity::controller::Interface::readAvbInterfaceDescriptor>(targetEntityID, handler, configurationIndex, avbInterfaceIndex);
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
		routeAemCommand<&entity::controller::Interface::readClockSourceDescriptor>(targetEntityID, handler, configurationIndex, clockSourceIndex);
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
		routeAemCommand<&entity::controller::Interface::readMemoryObjectDescriptor>(targetEntityID, handler, configurationIndex, memoryObjectIndex);
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
		routeAemCommand<&entity::controller::Interface::readLocaleDescriptor>(targetEntityID, handler, configurationIndex, localeIndex);
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
		routeAemCommand<&entity::controller::Interface::readStringsDescriptor>(targetEntityID, handler, configurationIndex, stringsIndex);
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
		routeAemCommand<&entity::controller::Interface::readStreamPortInputDescriptor>(targetEntityID, handler, configurationIndex, streamPortIndex);
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
		routeAemCommand<&entity::controller::Interface::readStreamPortOutputDescriptor>(targetEntityID, handler, configurationIndex, streamPortIndex);
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
		routeAemCommand<&entity::controller::Interface::readExternalPortInputDescriptor>(targetEntityID, handler, configurationIndex, externalPortIndex);
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
		routeAemCommand<&entity::controller::Interface::readExternalPortOutputDescriptor>(targetEntityID, handler, configurationIndex, externalPortIndex);
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
		routeAemCommand<&entity::controller::Interface::readInternalPortInputDescriptor>(targetEntityID, handler, configurationIndex, internalPortIndex);
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
		routeAemCommand<&entity::controller::Interface::readInternalPortOutputDescriptor>(targetEntityID, handler, configurationIndex, internalPortIndex);
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
		routeAemCommand<&entity::controller::Interface::readAudioClusterDescriptor>(targetEntityID, handler, configurationIndex, clusterIndex);
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
		routeAemCommand<&entity::controller::Interface::readAudioMapDescriptor>(targetEntityID, handler, configurationIndex, mapIndex);
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
		routeAemCommand<&entity::controller::Interface::readControlDescriptor>(targetEntityID, handler, configurationIndex, controlIndex);
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
		routeAemCommand<&entity::controller::Interface::readClockDomainDescriptor>(targetEntityID, handler, configurationIndex, clockDomainIndex);
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
		routeAemCommand<&entity::controller::Interface::readTimingDescriptor>(targetEntityID, handler, configurationIndex, timingIndex);
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
		routeAemCommand<&entity::controller::Interface::readPtpInstanceDescriptor>(targetEntityID, handler, configurationIndex, ptpInstanceIndex);
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
		routeAemCommand<&entity::controller::Interface::readPtpPortDescriptor>(targetEntityID, handler, configurationIndex, ptpPortIndex);
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
		routeAemCommand<&entity::controller::Interface::setConfiguration>(targetEntityID, handler, configurationIndex);
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
		routeAemCommand<&entity::controller::Interface::getConfiguration>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::setStreamInputFormat>(targetEntityID, handler, streamIndex, streamFormat);
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
		routeAemCommand<&entity::controller::Interface::getStreamInputFormat>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::setStreamOutputFormat>(targetEntityID, handler, streamIndex, streamFormat);
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
		routeAemCommand<&entity::controller::Interface::getStreamOutputFormat>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::getStreamPortInputAudioMap>(targetEntityID, handler, streamPortIndex, mapIndex);
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
		routeAemCommand<&entity::controller::Interface::getStreamPortOutputAudioMap>(targetEntityID, handler, streamPortIndex, mapIndex);
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
		routeAemCommand<&entity::controller::Interface::addStreamPortInputAudioMappings>(targetEntityID, handler, streamPortIndex, mappings);
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
		routeAemCommand<&entity::controller::Interface::addStreamPortOutputAudioMappings>(targetEntityID, handler, streamPortIndex, mappings);
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
		routeAemCommand<&entity::controller::Interface::removeStreamPortInputAudioMappings>(targetEntityID, handler, streamPortIndex, mappings);
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
		routeAemCommand<&entity::controller::Interface::removeStreamPortOutputAudioMappings>(targetEntityID, handler, streamPortIndex, mappings);
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
		routeAemCommand<&entity::controller::Interface::setStreamInputInfo>(targetEntityID, handler, streamIndex, info);
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
		routeAemCommand<&entity::controller::Interface::setStreamOutputInfo>(targetEntityID, handler, streamIndex, info);
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
		routeAemCommand<&entity::controller::Interface::getStreamInputInfo>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::getStreamOutputInfo>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::setEntityName>(targetEntityID, handler, entityName);
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
		routeAemCommand<&entity::controller::Interface::getEntityName>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::setEntityGroupName>(targetEntityID, handler, entityGroupName);
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
		routeAemCommand<&entity::controller::Interface::getEntityGroupName>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::setConfigurationName>(targetEntityID, handler, configurationIndex, configurationName);
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
		routeAemCommand<&entity::controller::Interface::getConfigurationName>(targetEntityID, handler, configurationIndex);
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
		routeAemCommand<&entity::controller::Interface::setAudioUnitName>(targetEntityID, handler, configurationIndex, audioUnitIndex, audioUnitName);
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
		routeAemCommand<&entity::controller::Interface::getAudioUnitName>(targetEntityID, handler, configurationIndex, audioUnitIndex);
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
		routeAemCommand<&entity::controller::Interface::setStreamInputName>(targetEntityID, handler, configurationIndex, streamIndex, streamInputName);
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
		routeAemCommand<&entity::controller::Interface::getStreamInputName>(targetEntityID, handler, configurationIndex, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::setStreamOutputName>(targetEntityID, handler, configurationIndex, streamIndex, streamOutputName);
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
		routeAemCommand<&entity::controller::Interface::getStreamOutputName>(targetEntityID, handler, configurationIndex, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::setJackInputName>(targetEntityID, handler, configurationIndex, jackIndex, jackInputName);
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
		routeAemCommand<&entity::controller::Interface::getJackInputName>(targetEntityID, handler, configurationIndex, jackIndex);
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
		routeAemCommand<&entity::controller::Interface::setJackOutputName>(targetEntityID, handler, configurationIndex, jackIndex, jackOutputName);
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
		routeAemCommand<&entity::controller::Interface::getJackOutputName>(targetEntityID, handler, configurationIndex, jackIndex);
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
		routeAemCommand<&entity::controller::Interface::setAvbInterfaceName>(targetEntityID, handler, configurationIndex, avbInterfaceIndex, avbInterfaceName);
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
		routeAemCommand<&entity::controller::Interface::getAvbInterfaceName>(targetEntityID, handler, configurationIndex, avbInterfaceIndex);
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
		routeAemCommand<&entity::controller::Interface::setClockSourceName>(targetEntityID, handler, configurationIndex, clockSourceIndex, clockSourceName);
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
		routeAemCommand<&entity::controller::Interface::getClockSourceName>(targetEntityID, handler, configurationIndex, clockSourceIndex);
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
		routeAemCommand<&entity::controller::Interface::setMemoryObjectName>(targetEntityID, handler, configurationIndex, memoryObjectIndex, memoryObjectName);
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
		routeAemCommand<&entity::controller::Interface::getMemoryObjectName>(targetEntityID, handler, configurationIndex, memoryObjectIndex);
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
		routeAemCommand<&entity::controller::Interface::setAudioClusterName>(targetEntityID, handler, configurationIndex, audioClusterIndex, audioClusterName);
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
		routeAemCommand<&entity::controller::Interface::getAudioClusterName>(targetEntityID, handler, configurationIndex, audioClusterIndex);
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
		routeAemCommand<&entity::controller::Interface::setControlName>(targetEntityID, handler, configurationIndex, controlIndex, controlName);
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
		routeAemCommand<&entity::controller::Interface::getControlName>(targetEntityID, handler, configurationIndex, controlIndex);
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
		routeAemCommand<&entity::controller::Interface::setClockDomainName>(targetEntityID, handler, configurationIndex, clockDomainIndex, clockDomainName);
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
		routeAemCommand<&entity::controller::Interface::getClockDomainName>(targetEntityID, handler, configurationIndex, clockDomainIndex);
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
		routeAemCommand<&entity::controller::Interface::setTimingName>(targetEntityID, handler, configurationIndex, timingIndex, timingName);
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
		routeAemCommand<&entity::controller::Interface::getTimingName>(targetEntityID, handler, configurationIndex, timingIndex);
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
		routeAemCommand<&entity::controller::Interface::setPtpInstanceName>(targetEntityID, handler, configurationIndex, ptpInstanceIndex, ptpInstanceName);
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
		routeAemCommand<&entity::controller::Interface::getPtpInstanceName>(targetEntityID, handler, configurationIndex, ptpInstanceIndex);
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
		routeAemCommand<&entity::controller::Interface::setPtpPortName>(targetEntityID, handler, configurationIndex, ptpPortIndex, ptpPortName);
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
		routeAemCommand<&entity::controller::Interface::getPtpPortName>(targetEntityID, handler, configurationIndex, ptpPortIndex);
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
		routeAemCommand<&entity::controller::Interface::setAssociation>(targetEntityID, handler, associationID);
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
		routeAemCommand<&entity::controller::Interface::getAssociation>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::setAudioUnitSamplingRate>(targetEntityID, handler, audioUnitIndex, samplingRate);
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
		routeAemCommand<&entity::controller::Interface::getAudioUnitSamplingRate>(targetEntityID, handler, audioUnitIndex);
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
		routeAemCommand<&entity::controller::Interface::setVideoClusterSamplingRate>(targetEntityID, handler, videoClusterIndex, samplingRate);
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
		routeAemCommand<&entity::controller::Interface::getVideoClusterSamplingRate>(targetEntityID, handler, videoClusterIndex);
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
		routeAemCommand<&entity::controller::Interface::setSensorClusterSamplingRate>(targetEntityID, handler, sensorClusterIndex, samplingRate);
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
		routeAemCommand<&entity::controller::Interface::getSensorClusterSamplingRate>(targetEntityID, handler, sensorClusterIndex);
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
		routeAemCommand<&entity::controller::Interface::setClockSource>(targetEntityID, handler, clockDomainIndex, clockSourceIndex);
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
		routeAemCommand<&entity::controller::Interface::getClockSource>(targetEntityID, handler, clockDomainIndex);
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
		routeAemCommand<&entity::controller::Interface::setControlValues>(targetEntityID, handler, controlIndex, controlValues);
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
		routeAemCommand<&entity::controller::Interface::getControlValues>(targetEntityID, handler, controlIndex);
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
		routeAemCommand<&entity::controller::Interface::startStreamInput>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::startStreamOutput>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::stopStreamInput>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::stopStreamOutput>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::getAvbInfo>(targetEntityID, handler, avbInterfaceIndex);
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
		routeAemCommand<&entity::controller::Interface::getAsPath>(targetEntityID, handler, avbInterfaceIndex);
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
		routeAemCommand<&entity::controller::Interface::getEntityCounters>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::getAvbInterfaceCounters>(targetEntityID, handler, avbInterfaceIndex);
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
		routeAemCommand<&entity::controller::Interface::getClockDomainCounters>(targetEntityID, handler, clockDomainIndex);
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
		routeAemCommand<&entity::controller::Interface::getStreamInputCounters>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::getStreamOutputCounters>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::reboot>(targetEntityID, handler);
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
		routeAemCommand<&entity::controller::Interface::rebootToFirmware>(targetEntityID, handler, memoryObjectIndex);
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
		routeAemCommand<&entity::controller::Interface::startOperation>(targetEntityID, handler, descriptorType, descriptorIndex, operationType, memoryBuffer);
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
		routeAemCommand<&entity::controller::Interface::abortOperation>(targetEntityID, handler, descriptorType, descriptorIndex, operationID);
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
		routeAemCommand<&entity::controller::Interface::setMemoryObjectLength>(targetEntityID, handler, configurationIndex, memoryObjectIndex, length);
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
		routeAemCommand<&entity::controller::Interface::getMemoryObjectLength>(targetEntityID, handler, configurationIndex, memoryObjectIndex);
	}
}

void ControllerVirtualProxy::getDynamicInfo(UniqueIdentifier const targetEntityID, entity::controller::DynamicInfoParameters const& parameters, GetDynamicInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, parameters, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getDynamicInfo(targetEntityID, parameters, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::getDynamicInfo>(targetEntityID, handler, parameters);
	}
}

void ControllerVirtualProxy::setMaxTransitTime(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, std::chrono::nanoseconds const& maxTransitTime, SetMaxTransitTimeHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, maxTransitTime, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setMaxTransitTime(targetEntityID, streamIndex, maxTransitTime, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::setMaxTransitTime>(targetEntityID, handler, streamIndex, maxTransitTime);
	}
}

void ControllerVirtualProxy::getMaxTransitTime(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetMaxTransitTimeHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getMaxTransitTime(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::getMaxTransitTime>(targetEntityID, handler, streamIndex);
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
		routeAemCommand<&entity::controller::Interface::addressAccess>(targetEntityID, handler, tlvs);
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
		routeAemCommand<&entity::controller::Interface::getMilanInfo>(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::setSystemUniqueID(UniqueIdentifier const targetEntityID, UniqueIdentifier const systemUniqueID, entity::model::AvdeccFixedString const& systemName, SetSystemUniqueIDHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, systemUniqueID, systemName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setSystemUniqueID(targetEntityID, systemUniqueID, systemName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::setSystemUniqueID>(targetEntityID, handler, systemUniqueID, systemName);
	}
}

void ControllerVirtualProxy::getSystemUniqueID(UniqueIdentifier const targetEntityID, GetSystemUniqueIDHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getSystemUniqueID(targetEntityID, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::getSystemUniqueID>(targetEntityID, handler);
	}
}

void ControllerVirtualProxy::setMediaClockReferenceInfo(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, std::optional<entity::model::MediaClockReferencePriority> const userPriority, std::optional<entity::model::AvdeccFixedString> const& domainName, SetMediaClockReferenceInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, clockDomainIndex, userPriority, domainName, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->setMediaClockReferenceInfo(targetEntityID, clockDomainIndex, userPriority, domainName, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::setMediaClockReferenceInfo>(targetEntityID, handler, clockDomainIndex, userPriority, domainName);
	}
}

void ControllerVirtualProxy::getMediaClockReferenceInfo(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, GetMediaClockReferenceInfoHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, clockDomainIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getMediaClockReferenceInfo(targetEntityID, clockDomainIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::getMediaClockReferenceInfo>(targetEntityID, handler, clockDomainIndex);
	}
}

void ControllerVirtualProxy::bindStream(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& talkerStream, entity::BindStreamFlags const flags, BindStreamHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, talkerStream, flags, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->bindStream(targetEntityID, streamIndex, talkerStream, flags, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::bindStream>(targetEntityID, handler, streamIndex, talkerStream, flags);
	}
}

void ControllerVirtualProxy::unbindStream(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, UnbindStreamHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->unbindStream(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::unbindStream>(targetEntityID, handler, streamIndex);
	}
}

void ControllerVirtualProxy::getStreamInputInfoEx(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, GetStreamInputInfoExHandler const& handler) const noexcept
{
	auto const isVirtual = isVirtualEntity(targetEntityID);
	if (isVirtual && _virtualInterface)
	{
		// Forward call to the virtual interface
		la::avdecc::ExecutorManager::getInstance().pushJob(_executorName,
			[this, targetEntityID, streamIndex, handler]()
			{
				auto const lg = std::lock_guard{ *_protocolInterface }; // Lock the ProtocolInterface as if we were called from the network thread
				_virtualInterface->getStreamInputInfoEx(targetEntityID, streamIndex, handler);
			});
	}
	else
	{
		// Forward call to real interface
		routeAemCommand<&entity::controller::Interface::getStreamInputInfoEx>(targetEntityID, handler, streamIndex);
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
		routeAcmpCommand<&entity::controller::Interface::connectStream>(listenerStream.entityID, handler, talkerStream, listenerStream);
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
		routeAcmpCommand<&entity::controller::Interface::disconnectStream>(listenerStream.entityID, handler, talkerStream, listenerStream);
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
		routeAcmpCommand<&entity::controller::Interface::disconnectTalkerStream>(talkerStream.entityID, handler, talkerStream, listenerStream);
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
		routeAcmpCommand<&entity::controller::Interface::getTalkerStreamState>(talkerStream.entityID, handler, talkerStream);
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
		routeAcmpCommand<&entity::controller::Interface::getListenerStreamState>(listenerStream.entityID, handler, listenerStream);
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
		routeAcmpCommand<&entity::controller::Interface::getTalkerStreamConnection>(talkerStream.entityID, handler, talkerStream, connectionIndex);
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
