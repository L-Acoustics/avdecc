/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file protocolInterface_c.cpp
* @author Christophe Calmejane
* @brief C bindings for la::avdecc::protocol::ProtocolInterface.
*/

#include <la/avdecc/internals/protocolInterface.hpp>
#include "la/avdecc/avdecc.h"
#include "localEntity_c.hpp"
#include "utils.hpp"

/* ************************************************************************** */
/* ProtocolInterface::Observer Bindings                                       */
/* ************************************************************************** */
class Observer final : public la::avdecc::protocol::ProtocolInterface::Observer
{
public:
	Observer(avdecc_protocol_interface_observer_p const observer, LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
		: _observer(observer)
		, _handle(handle)
	{
	}

	avdecc_protocol_interface_observer_p getObserver() const noexcept
	{
		return _observer;
	}

private:
	// la::avdecc::protocol::ProtocolInterface::Observer overrides
	/* **** Global notifications **** */
	virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const /*pi*/) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_observer->onTransportError, _handle);
	}

	/* **** Discovery notifications **** */
	virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& entity) noexcept override
	{
		auto const e = la::avdecc::bindings::fromCppToC::make_entity(entity);
		la::avdecc::utils::invokeProtectedHandler(_observer->onLocalEntityOnline, _handle, &e.entity);
	}
	virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_observer->onLocalEntityOffline, _handle, entityID);
	}
	virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& entity) noexcept override
	{
		auto const e = la::avdecc::bindings::fromCppToC::make_entity(entity);
		la::avdecc::utils::invokeProtectedHandler(_observer->onLocalEntityUpdated, _handle, &e.entity);
	}
	virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& entity) noexcept override
	{
		auto const e = la::avdecc::bindings::fromCppToC::make_entity(entity);
		la::avdecc::utils::invokeProtectedHandler(_observer->onRemoteEntityOnline, _handle, &e.entity);
	}
	virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_observer->onRemoteEntityOffline, _handle, entityID);
	}
	virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& entity) noexcept override
	{
		auto const e = la::avdecc::bindings::fromCppToC::make_entity(entity);
		la::avdecc::utils::invokeProtectedHandler(_observer->onRemoteEntityUpdated, _handle, &e.entity);
	}

	/* **** AECP notifications **** */
	virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override
	{
		static auto const s_Dispatch = std::unordered_map<la::avdecc::protocol::AecpMessageType, std::function<void(Observer* const obs, la::avdecc::protocol::Aecpdu const& aecpdu)>, la::avdecc::protocol::AecpMessageType::Hash>{
			{ la::avdecc::protocol::AecpMessageType::AemCommand,
				[](Observer* const obs, la::avdecc::protocol::Aecpdu const& aecpdu)
				{
					auto aecp = la::avdecc::bindings::fromCppToC::make_aem_aecpdu(static_cast<la::avdecc::protocol::AemAecpdu const&>(aecpdu));
					la::avdecc::utils::invokeProtectedHandler(obs->_observer->onAecpAemCommand, obs->_handle, &aecp);
				} },
			{ la::avdecc::protocol::AecpMessageType::AddressAccessCommand,
				[](Observer* const /*obs*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/)
				{
					//
				} },
			{ la::avdecc::protocol::AecpMessageType::VendorUniqueResponse,
				[](Observer* const /*obs*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/)
				{
					//
				} },
		};

		auto const& it = s_Dispatch.find(aecpdu.getMessageType());
		if (it != s_Dispatch.end())
		{
			it->second(this, aecpdu);
		}
	}
	virtual void onAecpAemUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::AemAecpdu const& aecpdu) noexcept override
	{
		auto aecp = la::avdecc::bindings::fromCppToC::make_aem_aecpdu(aecpdu);
		la::avdecc::utils::invokeProtectedHandler(_observer->onAecpAemUnsolicitedResponse, _handle, &aecp);
	}
	virtual void onAecpAemIdentifyNotification(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::AemAecpdu const& aecpdu) noexcept override
	{
		auto aecp = la::avdecc::bindings::fromCppToC::make_aem_aecpdu(aecpdu);
		la::avdecc::utils::invokeProtectedHandler(_observer->onAecpAemIdentifyNotification, _handle, &aecp);
	}

	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override
	{
		auto acmp = la::avdecc::bindings::fromCppToC::make_acmpdu(acmpdu);
		la::avdecc::utils::invokeProtectedHandler(_observer->onAcmpCommand, _handle, &acmp);
	}
	virtual void onAcmpResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override
	{
		auto acmp = la::avdecc::bindings::fromCppToC::make_acmpdu(acmpdu);
		la::avdecc::utils::invokeProtectedHandler(_observer->onAcmpResponse, _handle, &acmp);
	}

	/* **** Low level notifications (not supported by all kinds of ProtocolInterface), triggered before processing the pdu **** */
	virtual void onAdpduReceived(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Adpdu const& adpdu) noexcept override
	{
		auto acmp = la::avdecc::bindings::fromCppToC::make_adpdu(adpdu);
		la::avdecc::utils::invokeProtectedHandler(_observer->onAdpduReceived, _handle, &acmp);
	}
	virtual void onAecpduReceived(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override
	{
		static auto const s_Dispatch = std::unordered_map<la::avdecc::protocol::AecpMessageType, std::function<void(Observer* const obs, la::avdecc::protocol::Aecpdu const& aecpdu)>, la::avdecc::protocol::AecpMessageType::Hash>{
			{ la::avdecc::protocol::AecpMessageType::AemCommand,
				[](Observer* const obs, la::avdecc::protocol::Aecpdu const& aecpdu)
				{
					auto aecp = la::avdecc::bindings::fromCppToC::make_aem_aecpdu(static_cast<la::avdecc::protocol::AemAecpdu const&>(aecpdu));
					la::avdecc::utils::invokeProtectedHandler(obs->_observer->onAemAecpduReceived, obs->_handle, &aecp);
				} },
			{ la::avdecc::protocol::AecpMessageType::AddressAccessCommand,
				[](Observer* const /*obs*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/)
				{
					//
				} },
			{ la::avdecc::protocol::AecpMessageType::VendorUniqueResponse,
				[](Observer* const obs, la::avdecc::protocol::Aecpdu const& /*aecpdu*/)
				{
					la::avdecc::utils::invokeProtectedHandler(obs->_observer->onMvuAecpduReceived, obs->_handle, nullptr); // TODO: Create correct VU
				} },
		};

		auto const& it = s_Dispatch.find(aecpdu.getMessageType());
		if (it != s_Dispatch.end())
		{
			it->second(this, aecpdu);
		}
	}
	virtual void onAcmpduReceived(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override
	{
		auto acmp = la::avdecc::bindings::fromCppToC::make_acmpdu(acmpdu);
		la::avdecc::utils::invokeProtectedHandler(_observer->onAcmpduReceived, _handle, &acmp);
	}

	// Private members
	avdecc_protocol_interface_observer_p _observer{ nullptr };
	LA_AVDECC_PROTOCOL_INTERFACE_HANDLE _handle{ LA_AVDECC_INVALID_HANDLE };
	DECLARE_AVDECC_OBSERVER_GUARD(Observer);
};

/* ************************************************************************** */
/* ProtocolInterface APIs                                                     */
/* ************************************************************************** */
static la::avdecc::bindings::HandleManager<la::avdecc::protocol::ProtocolInterface::UniquePointer> s_ProtocolInterfaceManager{};
static la::avdecc::bindings::HandleManager<Observer*> s_ProtocolInterfaceObserverManager{};

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_create(avdecc_protocol_interface_type_t const protocolInterfaceType, avdecc_const_string_t interfaceName, LA_AVDECC_PROTOCOL_INTERFACE_HANDLE* const createdProtocolInterfaceHandle)
{
	try
	{
		*createdProtocolInterfaceHandle = s_ProtocolInterfaceManager.createObject(static_cast<la::avdecc::protocol::ProtocolInterface::Type>(protocolInterfaceType), std::string(interfaceName));
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(e.getError());
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_destroy(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		s_ProtocolInterfaceManager.destroyObject(handle);
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getMacAddress(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_mac_address_t* const address)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		la::avdecc::bindings::fromCppToC::set_macAddress(obj.getMacAddress(), *address);
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_shutdown(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		obj.shutdown();
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_registerObserver(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_interface_observer_p const observer)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);

		try
		{
			obj.registerObserver(&s_ProtocolInterfaceObserverManager.getObject(s_ProtocolInterfaceObserverManager.createObject(observer, handle)));
		}
		catch (...) // Same observer registered twice
		{
			return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_parameters);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_unregisterObserver(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_interface_observer_p const observer)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);

		// Search the observer matching our ProtocolInterface observer
		for (auto const& obsKV : s_ProtocolInterfaceObserverManager.getObjects())
		{
			auto const& obs = obsKV.second;
			if (obs->getObserver() == observer)
			{
				obj.unregisterObserver(obs.get());
				s_ProtocolInterfaceObserverManager.destroyObject(obsKV.first);
				break;
			}
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getDynamicEID(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_unique_identifier_t* const generatedEntityID)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		*generatedEntityID = obj.getDynamicEID();
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_releaseDynamicEID(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_unique_identifier_t const entityID)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		obj.releaseDynamicEID(la::avdecc::UniqueIdentifier{ entityID });
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_registerLocalEntity(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		try
		{
			auto& localEntity = getAggregateEntity(localEntityHandle);

			return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.registerLocalEntity(localEntity));
		}
		catch (...)
		{
			return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_unknown_local_entity);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_unregisterLocalEntity(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		try
		{
			auto& localEntity = getAggregateEntity(localEntityHandle);

			return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.unregisterLocalEntity(localEntity));
		}
		catch (...)
		{
			return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_unknown_local_entity);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_enableEntityAdvertising(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);

		try
		{
			auto& localEntity = getAggregateEntity(localEntityHandle);

			return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.enableEntityAdvertising(localEntity));
		}
		catch (...)
		{
			return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_unknown_local_entity);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_disableEntityAdvertising(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);

		try
		{
			auto& localEntity = getAggregateEntity(localEntityHandle);

			return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.disableEntityAdvertising(localEntity));
		}
		catch (...)
		{
			return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_unknown_local_entity);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_setEntityNeedsAdvertise(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle, avdecc_local_entity_advertise_flags_t const flags)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);

		try
		{
			auto& localEntity = getAggregateEntity(localEntityHandle);

			return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.setEntityNeedsAdvertise(localEntity, la::avdecc::bindings::fromCToCpp::convertLocalEntityAdvertiseFlags(flags)));
		}
		catch (...)
		{
			return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_unknown_local_entity);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_discoverRemoteEntities(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.discoverRemoteEntities());
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_discoverRemoteEntity(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_unique_identifier_t const entityID)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.discoverRemoteEntity(la::avdecc::UniqueIdentifier{ entityID }));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_isDirectMessageSupported(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return obj.isDirectMessageSupported();
		;
	}
	catch (...)
	{
		return false;
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAdpMessage(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_adpdu_cp const adpdu)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAdpMessage(la::avdecc::bindings::fromCToCpp::make_adpdu(adpdu)));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAemAecpMessage(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_aem_aecpdu_cp const aecpdu)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAecpMessage(la::avdecc::bindings::fromCToCpp::make_aem_aecpdu(aecpdu)));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAcmpMessage(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_acmpdu_cp const acmpdu)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAcmpMessage(la::avdecc::bindings::fromCToCpp::make_acmpdu(acmpdu)));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAemAecpCommand(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_aem_aecpdu_cp const aecpdu, avdecc_protocol_interfaces_send_aem_aecp_command_cb const onResult)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAecpCommand(la::avdecc::bindings::fromCToCpp::make_aem_aecpdu_unique(aecpdu),
			[onResult](la::avdecc::protocol::Aecpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error)
			{
				if (!error)
				{
					if (AVDECC_ASSERT_WITH_RET(response->getMessageType() == la::avdecc::protocol::AecpMessageType::AemResponse, "Received AECP is NOT an AEM Response"))
					{
						auto aecpdu = la::avdecc::bindings::fromCppToC::make_aem_aecpdu(static_cast<la::avdecc::protocol::AemAecpdu const&>(*response));
						la::avdecc::utils::invokeProtectedHandler(onResult, &aecpdu, la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(error));
					}
					else
					{
						la::avdecc::utils::invokeProtectedHandler(onResult, nullptr, static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error));
					}
				}
				else
				{
					la::avdecc::utils::invokeProtectedHandler(onResult, nullptr, la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(error));
				}
			}));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAemAecpResponse(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_aem_aecpdu_cp const aecpdu)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAecpResponse(la::avdecc::bindings::fromCToCpp::make_aem_aecpdu_unique(aecpdu)));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendMvuAecpCommand(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_mvu_aecpdu_cp const aecpdu, avdecc_protocol_interfaces_send_mvu_aecp_command_cb const onResult)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAecpCommand(la::avdecc::bindings::fromCToCpp::make_mvu_aecpdu_unique(aecpdu),
			[onResult](la::avdecc::protocol::Aecpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error)
			{
				if (!error)
				{
					if (AVDECC_ASSERT_WITH_RET(response->getMessageType() == la::avdecc::protocol::AecpMessageType::VendorUniqueResponse, "Received AECP is NOT a VU Response"))
					{
						auto const& vuaecpdu = static_cast<la::avdecc::protocol::VuAecpdu const&>(*response);
						if (AVDECC_ASSERT_WITH_RET(vuaecpdu.getProtocolIdentifier() == la::avdecc::protocol::MvuAecpdu::ProtocolID, "Received AECP is NOT a MVU Response"))
						{
							auto aecpdu = la::avdecc::bindings::fromCppToC::make_mvu_aecpdu(static_cast<la::avdecc::protocol::MvuAecpdu const&>(*response));
							la::avdecc::utils::invokeProtectedHandler(onResult, &aecpdu, la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(error));
						}
						else
						{
							la::avdecc::utils::invokeProtectedHandler(onResult, nullptr, static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error));
						}
					}
					else
					{
						la::avdecc::utils::invokeProtectedHandler(onResult, nullptr, static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error));
					}
				}
				else
				{
					la::avdecc::utils::invokeProtectedHandler(onResult, nullptr, la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(error));
				}
			}));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendMvuAecpResponse(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_mvu_aecpdu_cp const aecpdu)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAecpResponse(la::avdecc::bindings::fromCToCpp::make_mvu_aecpdu_unique(aecpdu)));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAcmpCommand(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_acmpdu_cp const acmpdu, avdecc_protocol_interfaces_send_acmp_command_cb const onResult)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAcmpCommand(la::avdecc::bindings::fromCToCpp::make_acmpdu_unique(acmpdu),
			[onResult](la::avdecc::protocol::Acmpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error)
			{
				if (!error)
				{
					auto acmpdu = la::avdecc::bindings::fromCppToC::make_acmpdu(*response);
					la::avdecc::utils::invokeProtectedHandler(onResult, &acmpdu, la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(error));
				}
				else
				{
					la::avdecc::utils::invokeProtectedHandler(onResult, nullptr, la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(error));
				}
			}));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAcmpResponse(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_acmpdu_cp const acmpdu)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return la::avdecc::bindings::fromCppToC::convertProtocolInterfaceErrorCode(obj.sendAcmpResponse(la::avdecc::bindings::fromCToCpp::make_acmpdu_unique(acmpdu)));
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_internal_error);
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_lock(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		obj.lock();
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_unlock(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		obj.unlock();
	}
	catch (...)
	{
		return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_protocol_interface_error_t>(avdecc_protocol_interface_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_isSelfLocked(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	try
	{
		auto& obj = s_ProtocolInterfaceManager.getObject(handle);
		return obj.isSelfLocked();
	}
	catch (...)
	{
		return avdecc_bool_false;
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_isSupportedProtocolInterfaceType(avdecc_protocol_interface_type_t const protocolInterfaceType)
{
	return la::avdecc::protocol::ProtocolInterface::isSupportedProtocolInterfaceType(static_cast<la::avdecc::protocol::ProtocolInterface::Type>(protocolInterfaceType));
}

LA_AVDECC_BINDINGS_C_API avdecc_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_typeToString(avdecc_protocol_interface_type_t const protocolInterfaceType)
{
	return strdup(la::avdecc::protocol::ProtocolInterface::typeToString(static_cast<la::avdecc::protocol::ProtocolInterface::Type>(protocolInterfaceType)).c_str());
}

LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_types_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getSupportedProtocolInterfaceTypes()
{
	return static_cast<avdecc_protocol_interface_types_t>(la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes().value());
}

/* ************************************************************************** */
/* ProtocolInterface private APIs                                             */
/* ************************************************************************** */
la::avdecc::protocol::ProtocolInterface& getProtocolInterface(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	return s_ProtocolInterfaceManager.getObject(handle);
}
