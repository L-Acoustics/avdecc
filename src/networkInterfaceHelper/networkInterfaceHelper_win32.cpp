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
* @file networkInterfaceHelper_win32.cpp
* @author Christophe Calmejane
*/

#include "networkInterfaceHelper_common.hpp"

#include <memory>
#include <cstdint> // std::uint8_t
#include <cstring> // memcpy
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <future>
#include <chrono>
#include <WinSock2.h>
#include <Iphlpapi.h>
#include <wbemidl.h>
#include <ws2tcpip.h>
#include <Mstcpip.h>
#include <ip2string.h>
#include <Windows.h>
#include <comutil.h>
#include <objbase.h>
#pragma comment(lib, "KERNEL32.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "NTDLL.lib")
#pragma comment(lib, "WBEMUUID.lib")

namespace la
{
namespace avdecc
{
namespace networkInterface
{
inline IPAddress::value_type_packed_v4 makePackedMaskV4(std::uint8_t CountBits) noexcept
{
	if (CountBits >= 32)
	{
		return IPAddress::value_type_packed_v4(-1);
	}
	return ~(~IPAddress::value_type_packed_v4(0) << CountBits);
}

inline IPAddress::value_type_packed_v4 makePackedMaskV6(std::uint8_t CountBits) noexcept
{
#pragma message("TODO: Use value_type_packed_v6")
	if (CountBits >= 128)
	{
		return IPAddress::value_type_packed_v4(-1);
	}
	return ~(~IPAddress::value_type_packed_v4(0) << CountBits);
}

static std::string wideCharToUTF8(PWCHAR const wide) noexcept
{
	// All APIs calling this method have to provide a NULL-terminated PWCHAR
	auto const wideLength = wcsnlen_s(wide, 1024); // Compute the size, in characters, of the wide string

	if (wideLength != 0)
	{
		auto const sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(wideLength), nullptr, 0, nullptr, nullptr);
		auto result = std::string(static_cast<std::string::size_type>(sizeNeeded), std::string::value_type{ 0 }); // Brace-initialization constructor prevents the use of {}

		if (WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(wideLength), result.data(), sizeNeeded, nullptr, nullptr) > 0)
		{
			return result;
		}
	}

	return {};
}

static Interface::Type getInterfaceType(IFTYPE const ifType) noexcept
{
	switch (ifType)
	{
		case IF_TYPE_ETHERNET_CSMACD:
			return Interface::Type::Ethernet;
		case IF_TYPE_SOFTWARE_LOOPBACK:
			return Interface::Type::Loopback;
		case IF_TYPE_IEEE80211:
			return Interface::Type::WiFi;
		default:
			break;
	}

	// Not supported type
	return Interface::Type::None;
}

class ComGuard final
{
public:
	ComGuard() noexcept {}
	~ComGuard()
	{
		CoUninitialize();
	}

	// Compiler auto-generated methods (move-only class)
	ComGuard(ComGuard const&) = delete;
	ComGuard(ComGuard&&) = delete;
	ComGuard& operator=(ComGuard const&) = delete;
	ComGuard& operator=(ComGuard&&) = default;
};

template<typename ComObject>
class ComObjectGuard final
{
public:
	ComObjectGuard(ComObject* ptr) noexcept
		: _ptr(ptr)
	{
	}
	~ComObjectGuard() noexcept
	{
		if (_ptr)
		{
			_ptr->Release();
		}
	}

private:
	ComObject* _ptr{ nullptr };
};

class VariantGuard final
{
public:
	VariantGuard(VARIANT* var) noexcept
		: _var(var)
	{
	}
	~VariantGuard() noexcept
	{
		if (_var)
		{
			VariantClear(_var);
		}
	}

private:
	VARIANT* _var{ nullptr };
};

static bool refreshInterfaces_WMI(Interfaces& interfaces) noexcept
{
	auto wmiSucceeded = false;

	// First pass, use WMI API to retrieve all the adapters and most of their information
	{
		// https://msdn.microsoft.com/en-us/library/Hh968170%28v=VS.85%29.aspx?f=255&MSPPError=-2147217396
		auto* locator = static_cast<IWbemLocator*>(nullptr);
		if (SUCCEEDED(CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&locator))))
		{
			auto const locatorGuard = ComObjectGuard{ locator };

			auto* service = static_cast<IWbemServices*>(nullptr);
			if (SUCCEEDED(locator->ConnectServer(L"root\\StandardCimv2", NULL, NULL, NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &service)))
			{
				auto const serviceGuard = ComObjectGuard{ service };

				// Set the security to Impersonate
				if (SUCCEEDED(CoSetProxyBlanket(service, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DEFAULT)))
				{
					auto* adapterEnumerator = static_cast<IEnumWbemClassObject*>(nullptr);
					if (SUCCEEDED(service->ExecQuery(L"WQL", L"SELECT * FROM MSFT_NetAdapter", WBEM_FLAG_FORWARD_ONLY, NULL, &adapterEnumerator)))
					{
						auto const adapterEnumeratorGuard = ComObjectGuard{ adapterEnumerator };

						while (adapterEnumerator)
						{
							auto* adapter = static_cast<IWbemClassObject*>(nullptr);
							auto retcnt = ULONG{ 0u };
							if (!SUCCEEDED(adapterEnumerator->Next(WBEM_INFINITE, 1L, reinterpret_cast<IWbemClassObject**>(&adapter), &retcnt)))
							{
								adapterEnumerator = nullptr;
								continue;
							}

							auto const adapterGuard = ComObjectGuard{ adapter };

							// No more adapters
							if (retcnt == 0)
							{
								adapterEnumerator = nullptr;
								continue;
							}

							// Check if interface is hidden
							{
								VARIANT hidden;
								if (!SUCCEEDED(adapter->Get(L"Hidden", 0, &hidden, NULL, NULL)))
								{
									continue;
								}

								auto const hiddenGuard = VariantGuard{ &hidden };
								// Only process visible adapters
								if (hidden.vt != VT_BOOL || hidden.boolVal)
								{
									continue;
								}
							}

							// Create a new Interface
							Interface i;

							// Get the type of interface
							{
								VARIANT interfaceType;
								// We absolutely need the type of interface
								if (!SUCCEEDED(adapter->Get(L"InterfaceType", 0, &interfaceType, NULL, NULL)))
								{
									continue;
								}

								auto const interfaceTypeGuard = VariantGuard{ &interfaceType };
								// vt seems to be VT_I4 although the doc says it should be VT_UINT or VT_UI4, thus not checking!

								auto const type = getInterfaceType(static_cast<IFTYPE>(interfaceType.uintVal));
								// Only process supported interface types
								if (type == Interface::Type::None)
								{
									continue;
								}

								i.type = type;
							}

							// Get the interface ID
							{
								VARIANT deviceID;
								// We absolutely need the interface ID
								if (!SUCCEEDED(adapter->Get(L"DeviceID", 0, &deviceID, NULL, NULL)))
								{
									continue;
								}

								if (deviceID.vt != VT_BSTR)
								{
									continue;
								}

								i.id = wideCharToUTF8(deviceID.bstrVal);
							}

							// Get the macAddress of the interface
							{
								VARIANT macAddress;
								// We absolutely need the macAddress of the interface
								if (!SUCCEEDED(adapter->Get(L"PermanentAddress", 0, &macAddress, NULL, NULL)))
								{
									continue;
								}

								auto const macAddressGuard = VariantGuard{ &macAddress };
								if (macAddress.vt != VT_BSTR)
								{
									continue;
								}

								auto const mac = wideCharToUTF8(macAddress.bstrVal);
								// Only process adapters with a MAC address
								if (mac.empty())
								{
									continue;
								}

								try
								{
									i.macAddress = stringToMacAddress(mac, '\0');
								}
								catch (...)
								{
									// Failed to convert macAddress, ignore this interface
									continue;
								}
							}

							// Optionally get the Description of the interface
							{
								VARIANT interfaceDescription;
								if (SUCCEEDED(adapter->Get(L"InterfaceDescription", 0, &interfaceDescription, NULL, NULL)))
								{
									auto const interfaceDescriptionGuard = VariantGuard{ &interfaceDescription };
									if (interfaceDescription.vt == VT_BSTR)
									{
										i.description = wideCharToUTF8(interfaceDescription.bstrVal);
									}
								}
							}

							// Optionally get the Friendly name of the interface
							{
								VARIANT friendlyName;
								if (SUCCEEDED(adapter->Get(L"Name", 0, &friendlyName, NULL, NULL)))
								{
									auto const friendlyNameGuard = VariantGuard{ &friendlyName };
									if (friendlyName.vt == VT_BSTR)
									{
										i.alias = wideCharToUTF8(friendlyName.bstrVal);
									}
								}
							}

							// Optionally get the Enabled State of the interface
							{
								VARIANT state;
								if (SUCCEEDED(adapter->Get(L"State", 0, &state, NULL, NULL)))
								{
									auto const stateGuard = VariantGuard{ &state };
									// vt seems to be VT_I4 although the doc says it should be VT_UINT or VT_UI4, thus not checking!

									//Unknown(0)
									//Present(1)
									//Started(2)
									//Disabled(3)

									i.isEnabled = state.uintVal == 2;
								}
								else
								{
									i.isEnabled = true; // In case we don't know, assume it's enabled
								}
							}

							// Optionally get the Operational Status of the interface
							{
								VARIANT operationalStatus;
								if (SUCCEEDED(adapter->Get(L"InterfaceOperationalStatus", 0, &operationalStatus, NULL, NULL)))
								{
									auto const operationalStatusGuard = VariantGuard{ &operationalStatus };
									// vt seems to be VT_I4 although the doc says it should be VT_UINT or VT_UI4, thus not checking!

									i.isConnected = static_cast<IF_OPER_STATUS>(operationalStatus.uintVal) == IfOperStatusUp;
								}
								else
								{
									i.isConnected = true; // In case we don't know, assume it's connected
								}
							}

							{
								VARIANT isVirtual;
								if (SUCCEEDED(adapter->Get(L"Virtual", 0, &isVirtual, NULL, NULL)))
								{
									auto const isVirtualGuard = VariantGuard{ &isVirtual };
									if (isVirtual.vt == VT_BOOL)
									{
										i.isVirtual = isVirtual.boolVal;
									}
								}
							}

							// Everything OK, save this interface
							interfaces[i.id] = i;
						}

						// Only if WMI succeeded that we will try it again
						wmiSucceeded = true;
					}
				}
			}
		}
	}

	// Second pass, get the IP configuration for all discovered adapters
	if (wmiSucceeded)
	{
		ULONG ulSize = 0;
		ULONG family = AF_INET; // AF_UNSPEC for ipV4 and ipV6, AF_INET6 for ipV6 only

		GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &ulSize); // Make an initial call to get the needed size to allocate
		auto buffer = std::make_unique<std::uint8_t[]>(ulSize);
		if (GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, (PIP_ADAPTER_ADDRESSES)buffer.get(), &ulSize) != ERROR_SUCCESS)
		{
			return false;
		}

		for (auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get()); adapter != nullptr; adapter = adapter->Next)
		{
			// Only process adapters already discovered by WMI
			auto intfcIt = interfaces.find(adapter->AdapterName);
			if (intfcIt == interfaces.end())
			{
				// Special case for the loopback interface that is not discovered by WMI, add it now
				if (adapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
				{
					continue;
				}
				// Create a loopback Interface and setup information
				auto i = Interface{};

				i.id = adapter->AdapterName;
				i.description = wideCharToUTF8(adapter->Description);
				i.alias = wideCharToUTF8(adapter->FriendlyName);
				if (adapter->PhysicalAddressLength == i.macAddress.size())
					std::memcpy(i.macAddress.data(), adapter->PhysicalAddress, adapter->PhysicalAddressLength);
				i.type = Interface::Type::Loopback;
				i.isConnected = adapter->OperStatus == IfOperStatusUp;
				i.isEnabled = true;
				i.isVirtual = true;

				// Add it to the list, so we can get its IP addresses
				intfcIt = interfaces.emplace(adapter->AdapterName, std::move(i)).first;
			}
			auto& i = intfcIt->second;

			// Retrieve IP addresses
			for (auto ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				auto const isIPv6 = ua->Address.lpSockaddr->sa_family == AF_INET6;
				if (isIPv6)
				{
					std::array<char, 46> ip;
					RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ua->Address.lpSockaddr)->sin6_addr, ip.data());
					try
					{
						i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ std::string(ip.data()) }, IPAddress{ makePackedMaskV6(ua->OnLinkPrefixLength) } });
					}
					catch (...)
					{
					}
				}
				else
				{
					i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr)->sin_addr.S_un.S_addr }, IPAddress{ makePackedMaskV4(ua->OnLinkPrefixLength) } });
				}
			}

			// Retrieve Gateways
			for (auto ga = adapter->FirstGatewayAddress; ga != nullptr; ga = ga->Next)
			{
				auto const isIPv6 = ga->Address.lpSockaddr->sa_family == AF_INET6;
				if (isIPv6)
				{
					std::array<char, 46> ip;
					RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ga->Address.lpSockaddr)->sin6_addr, ip.data());
					try
					{
						i.gateways.push_back(IPAddress{ std::string(ip.data()) });
					}
					catch (...)
					{
					}
				}
				else
				{
					i.gateways.push_back(IPAddress{ reinterpret_cast<struct sockaddr_in*>(ga->Address.lpSockaddr)->sin_addr.S_un.S_addr });
				}
			}
		}
	}

	return wmiSucceeded;
}

static void refreshInterfaces_WinAPI(Interfaces& interfaces) noexcept
{
	// Unfortunately, GetAdaptersAddresses (even GetAdaptersInfo) is very limited and can only retrieve NICs that have IP enabled (and are active)

	ULONG ulSize = 0;
	ULONG family = AF_INET; // AF_UNSPEC for ipV4 and ipV6, AF_INET6 for ipV6 only

	GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &ulSize); // Make an initial call to get the needed size to allocate
	auto buffer = std::make_unique<std::uint8_t[]>(ulSize);
	if (GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, (PIP_ADAPTER_ADDRESSES)buffer.get(), &ulSize) != ERROR_SUCCESS)
	{
		return;
	}

	for (auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get()); adapter != nullptr; adapter = adapter->Next)
	{
		auto const type = getInterfaceType(adapter->IfType);
		// Only process supported interface types
		if (type == Interface::Type::None)
		{
			continue;
		}

		auto i = Interface{};

		i.id = adapter->AdapterName;
		i.description = wideCharToUTF8(adapter->Description);
		i.alias = wideCharToUTF8(adapter->FriendlyName);
		if (adapter->PhysicalAddressLength == i.macAddress.size())
			std::memcpy(i.macAddress.data(), adapter->PhysicalAddress, adapter->PhysicalAddressLength);
		i.type = type;
		i.isEnabled = true; // GetAdaptersAddresses (even GetAdaptersInfo) can only retrieve NICs that are active, so it's always Enabled
		i.isConnected = adapter->OperStatus == IfOperStatusUp;
		i.isVirtual = type == Interface::Type::Loopback; // GetAdaptersAddresses (even GetAdaptersInfo) cannot get the Virtual information (that WMI can), so only define Loopback as virtual

		// Retrieve IP addresses
		for (auto ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
		{
			auto const isIPv6 = ua->Address.lpSockaddr->sa_family == AF_INET6;
			if (isIPv6)
			{
				std::array<char, 46> ip;
				RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ua->Address.lpSockaddr)->sin6_addr, ip.data());
				try
				{
					i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ std::string(ip.data()) }, IPAddress{ makePackedMaskV6(ua->OnLinkPrefixLength) } });
				}
				catch (...)
				{
				}
			}
			else
			{
				i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr)->sin_addr.S_un.S_addr }, IPAddress{ makePackedMaskV4(ua->OnLinkPrefixLength) } });
			}
		}

		// Retrieve Gateways
		for (auto ga = adapter->FirstGatewayAddress; ga != nullptr; ga = ga->Next)
		{
			auto const isIPv6 = ga->Address.lpSockaddr->sa_family == AF_INET6;
			if (isIPv6)
			{
				std::array<char, 46> ip;
				RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ga->Address.lpSockaddr)->sin6_addr, ip.data());
				try
				{
					i.gateways.push_back(IPAddress{ std::string(ip.data()) });
				}
				catch (...)
				{
				}
			}
			else
			{
				i.gateways.push_back(IPAddress{ reinterpret_cast<struct sockaddr_in*>(ga->Address.lpSockaddr)->sin_addr.S_un.S_addr });
			}
		}

		// Add it to the list, so we can get its IP addresses
		interfaces.emplace(adapter->AdapterName, std::move(i));
	}
}

class WMIInitializer final
{
public:
	WMIInitializer()
	{
		// Using CreateThread as it's the only safe option to create a thread in a DllMain
		_thread = CreateThread(nullptr, 0, threadFunction, this, 0, nullptr);
	}

	~WMIInitializer() noexcept
	{
		_shouldTerminate = true;
		if (_thread)
		{
			WaitForSingleObject(_thread, INFINITE);
			_thread = nullptr;
		}
	}

	static DWORD WINAPI threadFunction(LPVOID lParam) noexcept
	{
		auto* self = static_cast<WMIInitializer*>(lParam);

		utils::setCurrentThreadName("networkInterfaceHelper::InterfacesPolling");

		// Try to initialize COM
		if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
		{
			self->_comGuard.emplace();
		}

		auto previousList = Interfaces{};
		auto nextCheck = std::chrono::time_point<std::chrono::system_clock>{};
		while (!self->_shouldTerminate)
		{
			auto const now = std::chrono::system_clock::now();
			if (now >= nextCheck)
			{
				auto newList = Interfaces{};

				if (self->_comGuard.has_value())
				{
					if (!refreshInterfaces_WMI(newList))
					{
						// WMI failed, never try this again
						self->_comGuard.reset();

						// Clear the list, just in case we managed to get some information
						newList.clear();
					}
				}

				// Cannot use WMI, use an alternative method (less powerful)
				if (!self->_comGuard.has_value())
				{
					refreshInterfaces_WinAPI(newList);
				}

				// Process previous list and check if some property changed
				for (auto const& [name, previousIntfc] : previousList)
				{
					auto const newIntfcIt = newList.find(name);
					if (newIntfcIt != newList.end())
					{
						auto const& newIntfc = newIntfcIt->second;
						if (previousIntfc.isEnabled != newIntfc.isEnabled)
						{
							onEnabledStateChanged(name, newIntfc.isEnabled);
						}
						if (previousIntfc.isConnected != newIntfc.isConnected)
						{
							onConnectedStateChanged(name, newIntfc.isConnected);
						}
					}
				}

				// Copy the list before it's moved
				previousList = newList;

				// Check for change in Interface count
				onNewInterfacesList(std::move(newList));

				// Set that we enumerated at least once
				self->_enumeratedOnce = true;
				self->_syncCondVar.notify_all();

				// Setup next check time
				nextCheck = now + std::chrono::milliseconds(1000);
			}

			// Wait a little bit so we don't burn the CPU
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		// Terminate the thread properly
		ExitThread(0);
	}

	void waitForFirstEnumeration() noexcept
	{
		// Check if enumeration was run at least once
		if (!_enumeratedOnce)
		{
			// Never ran, wait for it to run at least once
			auto lock = std::unique_lock{ _syncLock };
			_syncCondVar.wait(lock,
				[this]
				{
					return !!_enumeratedOnce;
				});
		}
	}

private:
	HANDLE _thread{ nullptr };
	std::mutex _syncLock{};
	std::condition_variable _syncCondVar{};
	std::atomic_bool _enumeratedOnce{ false };
	bool _shouldTerminate{ false };
	std::thread _enumerateThread{};
	std::optional<ComGuard> _comGuard{ std::nullopt }; // ComGuard to be sure CoUninitialize is called upon program termination
};

static auto s_wmiInitializer = WMIInitializer{};


void waitForFirstEnumeration() noexcept
{
	s_wmiInitializer.waitForFirstEnumeration();
}

void onFirstObserverRegistered() noexcept {}

void onLastObserverUnregistered() noexcept {}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
