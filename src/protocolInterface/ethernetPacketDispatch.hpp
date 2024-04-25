/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file ethernetPacketDispatch.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/protocolAdpdu.hpp"
#include "la/avdecc/internals/protocolAcmpdu.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/executor.hpp"

#include "stateMachine/stateMachineManager.hpp"
#include "logHelper.hpp"

#include <cstdint>

namespace la
{
namespace avdecc
{
namespace protocol
{
template<class BaseClass>
class EthernetPacketDispatcher
{
public:
	EthernetPacketDispatcher(BaseClass* const self, stateMachine::Manager& stateMachineManager)
		: _self{ self }
		, _stateMachineManager{ stateMachineManager }
	{
	}

	void dispatchAvdeccMessage(std::uint8_t const* const pkt_data, size_t const pkt_len, EtherLayer2 const& etherLayer2) const noexcept
	{
		try
		{
			// Read Avtpdu SubType and ControlData (which is remapped to MessageType for all 1722.1 messages)
			std::uint8_t const subType = pkt_data[0] & 0x7f;
			std::uint8_t const controlData = pkt_data[1] & 0x7f;

			// Create a deserialization buffer
			auto des = DeserializationBuffer(pkt_data, pkt_len);

			switch (subType)
			{
				/* ADP Message */
				case AvtpSubType_Adp:
				{
					auto adpdu = Adpdu::create();
					auto& adp = static_cast<Adpdu&>(*adpdu);

					// Fill EtherLayer2
					adp.setSrcAddress(etherLayer2.getSrcAddress());
					adp.setDestAddress(etherLayer2.getDestAddress());
					// Then deserialize Avtp control
					deserialize<AvtpduControl>(&adp, des);
					// Then deserialize Adp
					deserialize<Adpdu>(&adp, des);

					// Low level notification
					_self->template notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAdpduReceived, _self, adp);

					// Forward to our state machine
					_stateMachineManager.processAdpdu(adp);
					break;
				}

				/* AECP Message */
				case AvtpSubType_Aecp:
				{
					auto const messageType = static_cast<AecpMessageType>(controlData);

					static std::unordered_map<AecpMessageType, std::function<Aecpdu::UniquePointer(BaseClass* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)>, AecpMessageType::Hash> s_Dispatch{
						{ AecpMessageType::AemCommand,
							[](BaseClass* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AemAecpdu::create(false);
							} },
						{ AecpMessageType::AemResponse,
							[](BaseClass* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AemAecpdu::create(true);
							} },
						{ AecpMessageType::AddressAccessCommand,
							[](BaseClass* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AaAecpdu::create(false);
							} },
						{ AecpMessageType::AddressAccessResponse,
							[](BaseClass* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AaAecpdu::create(true);
							} },
						{ AecpMessageType::VendorUniqueCommand,
							[](BaseClass* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)
							{
								// We have to retrieve the ProtocolID to dispatch
								auto const protocolIdentifierOffset = AvtpduControl::HeaderLength + Aecpdu::HeaderLength;
								if (pkt_len >= (protocolIdentifierOffset + VuAecpdu::ProtocolIdentifier::Size))
								{
									auto protocolIdentifier = VuAecpdu::ProtocolIdentifier::ArrayType{};
									std::memcpy(protocolIdentifier.data(), pkt_data + protocolIdentifierOffset, VuAecpdu::ProtocolIdentifier::Size);

									auto const vuProtocolID = VuAecpdu::ProtocolIdentifier{ protocolIdentifier };
									auto* vuDelegate = pi->getVendorUniqueDelegate(vuProtocolID);
									if (vuDelegate)
									{
										// VendorUnique Commands are always handled by the VendorUniqueDelegate
										auto aecpdu = vuDelegate->createAecpdu(vuProtocolID, false);
										auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

										// Deserialize the aecp message
										deserializeAecpMessage(etherLayer2, des, vuAecp);

										// Low level notification
										pi->template notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, pi, vuAecp);

										// Forward to the delegate
										vuDelegate->onVuAecpCommand(pi, vuProtocolID, vuAecp);

										// Return empty Aecpdu so that it's not processed by the StateMachineManager
									}
									else
									{
										LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Command for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
									}
								}
								else
								{
									LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Invalid VendorUnique Command received. Not enough bytes in the message to hold ProtocolIdentifier");
								}

								return Aecpdu::UniquePointer{ nullptr, nullptr };
							} },
						{ AecpMessageType::VendorUniqueResponse,
							[](BaseClass* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)
							{
								// We have to retrieve the ProtocolID to dispatch
								auto const protocolIdentifierOffset = AvtpduControl::HeaderLength + Aecpdu::HeaderLength;
								if (pkt_len >= (protocolIdentifierOffset + VuAecpdu::ProtocolIdentifier::Size))
								{
									auto protocolIdentifier = VuAecpdu::ProtocolIdentifier::ArrayType{};
									std::memcpy(protocolIdentifier.data(), pkt_data + protocolIdentifierOffset, VuAecpdu::ProtocolIdentifier::Size);

									auto const vuProtocolID = VuAecpdu::ProtocolIdentifier{ protocolIdentifier };
									auto* vuDelegate = pi->getVendorUniqueDelegate(vuProtocolID);
									if (vuDelegate)
									{
										auto aecpdu = vuDelegate->createAecpdu(vuProtocolID, true);

										// Are the messages handled by the VendorUniqueDelegate itself
										if (!vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
										{
											auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

											// Deserialize the aecp message
											deserializeAecpMessage(etherLayer2, des, vuAecp);

											// Low level notification
											pi->template notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, pi, vuAecp);

											// Forward to the delegate
											vuDelegate->onVuAecpResponse(pi, vuProtocolID, vuAecp);

											// Return empty Aecpdu so that it's not processed by the StateMachineManager
											aecpdu.reset(nullptr);
										}

										return aecpdu;
									}
									else
									{
										LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Response for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
									}
								}
								else
								{
									LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Invalid VendorUnique Command received. Not enough bytes in the message to hold ProtocolIdentifier");
								}

								return Aecpdu::UniquePointer{ nullptr, nullptr };
							} },
					};

					auto const& it = s_Dispatch.find(messageType);
					if (it == s_Dispatch.end())
						return; // Unsupported AECP message type

					// Create aecpdu frame based on message type
					auto aecpdu = it->second(_self, etherLayer2, des, pkt_data, pkt_len);

					if (aecpdu != nullptr)
					{
						auto& aecp = static_cast<Aecpdu&>(*aecpdu);

						// Deserialize the aecp message
						deserializeAecpMessage(etherLayer2, des, aecp);

						// Low level notification
						_self->template notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, _self, aecp);

						// Forward to our state machine
						_stateMachineManager.processAecpdu(aecp);
					}
					break;
				}

				/* ACMP Message */
				case AvtpSubType_Acmp:
				{
					auto acmpdu = Acmpdu::create();
					auto& acmp = static_cast<Acmpdu&>(*acmpdu);

					// Fill EtherLayer2
					acmp.setSrcAddress(etherLayer2.getSrcAddress());
					static_cast<EtherLayer2&>(*acmpdu).setDestAddress(etherLayer2.getDestAddress()); // Fill dest address, even if we know it's always the MultiCast address
					// Then deserialize Avtp control
					deserialize<AvtpduControl>(&acmp, des);
					// Then deserialize Acmp
					deserialize<Acmpdu>(&acmp, des);

					// Low level notification
					_self->template notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpduReceived, _self, acmp);

					// Forward to our state machine
					_stateMachineManager.processAcmpdu(acmp);
					break;
				}

				/* MAAP Message */
				case AvtpSubType_Maap:
				{
					break;
				}
				default:
					return;
			}
		}
		catch ([[maybe_unused]] std::invalid_argument const& e)
		{
			LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, std::string("ProtocolInterfacePCap: Packet dropped: ") + e.what());
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unknown exception");
			LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "ProtocolInterfacePCap: Packet dropped due to unknown exception");
		}
	}

private:
	static void deserializeAecpMessage(EtherLayer2 const& etherLayer2, Deserializer& des, Aecpdu& aecp)
	{
		// Fill EtherLayer2
		aecp.setSrcAddress(etherLayer2.getSrcAddress());
		aecp.setDestAddress(etherLayer2.getDestAddress());
		// Then deserialize Avtp control
		deserialize<AvtpduControl>(&aecp, des);
		// Then deserialize Aecp
		deserialize<Aecpdu>(&aecp, des);
	}

	BaseClass* _self{ nullptr };
	stateMachine::Manager& _stateMachineManager;
};

} // namespace protocol
} // namespace avdecc
} // namespace la
