/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file controllerEntityImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/controllerEntity.hpp"
#include "entityImpl.hpp"
#include "protocolInterface/protocolInterface.hpp"
#include "protocol/protocolAemAecpdu.hpp"
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace entity
{

/* ************************************************************************** */
/* ControllerEntityImpl                                                       */
/* ************************************************************************** */
class ControllerEntityImpl : public LocalEntityImpl<ControllerEntity>, public protocol::ProtocolInterface::Observer
{
private:
	friend class LocalEntityGuard<ControllerEntityImpl>;

	/* ************************************************************************** */
	/* ControllerEntityImpl life cycle                                            */
	/* ************************************************************************** */
	ControllerEntityImpl(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, ControllerEntity::Delegate* const delegate);
	~ControllerEntityImpl() noexcept;

private:
	struct AnswerCallback
	{
		using Callback = std::function<void() noexcept>;
		Callback onAnswer{ nullptr };
		// Constructors
		AnswerCallback() = default;
		template<typename T>
		AnswerCallback(T const& f) : onAnswer(reinterpret_cast<Callback const&>(f))
		{
		}
		// Call operator
		template<typename T, typename... Ts>
		void invoke(Ts&&... params) const noexcept
		{
			if (onAnswer)
			{
				try
				{
					reinterpret_cast<T const&>(onAnswer)(std::forward<Ts>(params)...);
				}
				catch (...)
				{
					// Ignore throws in user handler
				}
			}
		}
	};

	using DiscoveredEntities = std::unordered_map<UniqueIdentifier, entity::DiscoveredEntity>;
	using OnAECPErrorCallback = std::function<void(ControllerEntity::AemCommandStatus const error) noexcept>;
	using OnACMPErrorCallback = std::function<void(ControllerEntity::ControlStatus const error) noexcept>;
	using OnErrorCallback = std::function<void() noexcept>;

	/* ************************************************************************** */
	/* ControllerEntityImpl internal methods                                      */
	/* ************************************************************************** */
	template<typename T, typename... Ts>
	static OnAECPErrorCallback makeAECPErrorHandler(T const& handler, ControllerEntity const* const controller, Ts&&... params)
	{
		if (handler)
			return std::bind(handler, controller, std::forward<Ts>(params)...);
		// No handler specified, return an empty handler
		return [](ControllerEntity::AemCommandStatus const /*error*/)
		{
		};
	}
	template<typename T, typename... Ts>
	static OnACMPErrorCallback makeACMPErrorHandler(T const& handler, ControllerEntity const* const controller, Ts&&... params)
	{
		if (handler)
			return std::bind(handler, controller, std::forward<Ts>(params)...);
		// No handler specified, return an empty handler
		return [](ControllerEntity::ControlStatus const /*error*/)
		{
		};
	}

	AemCommandStatus convertErrorToAemCommandStatus(protocol::ProtocolInterface::Error const error) const noexcept;
	ControlStatus convertErrorToControlStatus(protocol::ProtocolInterface::Error const error) const noexcept;
	void sendAemCommand(UniqueIdentifier const targetEntityID, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, OnAECPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept;
	void processAemResponse(protocol::Aecpdu const* const response, OnAECPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept;
	void sendAemResponse(protocol::AemAecpdu const& commandAem, protocol::AecpStatus const status, void const* const payload, size_t const payloadLength) const noexcept;
	void sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, OnACMPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept;
	void processAcmpResponse(protocol::Acmpdu const* const response, OnACMPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback, bool const sniffed) const noexcept;

	/* ************************************************************************** */
	/* ControllerEntity overrides                                                 */
	/* ************************************************************************** */
	/* Discovery Protocol (ADP) */
	/* Enumeration and Control Protocol (AECP) */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept override;
	virtual void unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept override;
	virtual void queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept override;
	virtual void queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept override;
	virtual void registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept override;
	virtual void unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept override;
	virtual void readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept override;
	virtual void readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept override;
	virtual void readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept override;
	virtual void readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept override;
	virtual void readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void getStreamInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, entity::model::MapIndex const mapIndex, GetStreamInputAudioMapHandler const& handler) const noexcept override;
	virtual void getStreamOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, entity::model::MapIndex const mapIndex, GetStreamOutputAudioMapHandler const& handler) const noexcept override;
	virtual void addStreamInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, std::vector<model::AudioMapping> const& mappings, AddStreamInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, std::vector<model::AudioMapping> const& mappings, RemoveStreamInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept override;
	virtual void getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& entityGroupName, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept override;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept override;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept override;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept override;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept override;
	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept override;
	/* Other methods */
	virtual void setDelegate(Delegate* const delegate) noexcept override;
	Delegate* getDelegate() const noexcept;

	/* ************************************************************************** */
	/* protocol::ProtocolInterface::Observer overrides                            */
	/* ************************************************************************** */
	/* **** Global notifications **** */
	virtual void onTransportError(la::avdecc::protocol::ProtocolInterface const* const pi) noexcept override;
	/* **** Discovery notifications **** */
	virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface const* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface const* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	/* **** AECP notifications **** */
	virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface const* const pi, entity::LocalEntity const& entity, protocol::Aecpdu const& aecpdu) noexcept override;
	virtual void onAecpUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface const* const pi, entity::LocalEntity const& entity, protocol::Aecpdu const& aecpdu) noexcept override;
	/* **** ACMP notifications **** */
	virtual void onAcmpSniffedCommand(la::avdecc::protocol::ProtocolInterface const* const pi, entity::LocalEntity const& entity, protocol::Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpSniffedResponse(la::avdecc::protocol::ProtocolInterface const* const pi, entity::LocalEntity const& entity, protocol::Acmpdu const& acmpdu) noexcept override;

	/* ************************************************************************** */
	/* Internal variables                                                         */
	/* ************************************************************************** */
	ControllerEntity::Delegate* _delegate{ nullptr };
	bool _shouldTerminate{ false };
	mutable std::mutex _lockDiscoveredEntities{};
	DiscoveredEntities _discoveredEntities{};
	std::thread _discoveryThread{};
};

} // namespace entity
} // namespace avdecc
} // namespace la
