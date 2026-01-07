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
* @file protocolInterface_pcap_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/executor.hpp>
#include <la/avdecc/internals/protocolMvuAecpdu.hpp>
#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

// Internal API
#include "protocolInterface/protocolInterface_pcap.hpp"

#include <gtest/gtest.h>
#include <future>
#include <chrono>
#include <string>
#include <memory>
#include <iostream>
#include <string>

static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

static la::networkInterface::Interface getFirstInterface()
{
	auto interface = la::networkInterface::Interface{};

	// COMMENTED CODE TO FORCE ALL TESTS USING THIS TO BE DISABLED AUTOMATICALLY ;)

	//la::networkInterface::enumerateInterfaces(
	//	[&interface](la::networkInterface::Interface const& intfc)
	//	{
	//		if (intfc.type == la::networkInterface::Interface::Type::Ethernet && intfc.isEnabled && interface.type == la::networkInterface::Interface::Type::None)
	//		{
	//			interface = intfc;
	//		}
	//	});
	return interface;
}

TEST(ProtocolInterfacePCap, InvalidName)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		std::unique_ptr<la::avdecc::protocol::ProtocolInterfacePcap>(la::avdecc::protocol::ProtocolInterfacePcap::createRawProtocolInterfacePcap("", DefaultExecutorName));
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InterfaceNotFound, e.getError());
	}
}

TEST(ProtocolInterfacePCap, TransportError)
{
	static std::promise<void> entityOnlinePromise;
	static std::promise<void> completedPromise;

	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	{
	private:
		// la::avdecc::protocol::ProtocolInterface::Observer overrides
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const pi) noexcept override
		{
			std::this_thread::sleep_for(std::chrono::seconds(15)); // Wait for an entity to go offline
			// Now we are sure ProtocolInterface (from avdecc::CommandStateMachine thread) wants to acquire the observers lock, but we currently own this lock
			// So let's call something that wants to acquire the CommandStateMachine and see if we deadlock or not
			pi->lock(); // This will use CSM's lock
			pi->unlock();
		}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			static auto done{ false };
			if (!done)
			{
				entityOnlinePromise.set_value();
				std::cout << "Found an entity, now trigger a transport error by disabling the network interface\n";
				done = true;
			}
		}
		virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override
		{
			completedPromise.set_value();
		}
		DECLARE_AVDECC_OBSERVER_GUARD(Observer);
	};

	auto const interface = getFirstInterface();
	if (interface.type != la::networkInterface::Interface::Type::None)
	{
		std::cout << "Using interface " << interface.alias << std::endl;

		Observer obs;
		auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfacePcap>(la::avdecc::protocol::ProtocolInterfacePcap::createRawProtocolInterfacePcap(interface.id, DefaultExecutorName));
		intfc->registerObserver(&obs);

		auto status = entityOnlinePromise.get_future().wait_for(std::chrono::seconds(5));
		ASSERT_NE(std::future_status::timeout, status) << "Failed to detect an online entity... stopping the test";

		status = completedPromise.get_future().wait_for(std::chrono::seconds(60));
		ASSERT_NE(std::future_status::timeout, status) << "Either deadlock or you didn't follow instructions quickly enough";
	}
}

namespace
{
class INTEGRATION_ProtocolInterfacePCap_F : public ::testing::Test
{
public:
	virtual void SetUp() override
	{
		// Search a valid NetworkInterface, the first active one actually
		auto networkInterfaceID = std::string{};
		la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
			[&networkInterfaceID](la::networkInterface::Interface const& intfc)
			{
				if (!networkInterfaceID.empty())
				{
					return;
				}
				if (intfc.type == la::networkInterface::Interface::Type::Ethernet && intfc.isConnected && !intfc.isVirtual)
				{
					networkInterfaceID = intfc.id;
				}
			});

		ASSERT_FALSE(networkInterfaceID.empty()) << "No valid NetworkInterface found";
		_ew = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
		_pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfacePcap>(la::avdecc::protocol::ProtocolInterfacePcap::createRawProtocolInterfacePcap(networkInterfaceID, DefaultExecutorName));
	}

	virtual void TearDown() override
	{
		_pi.reset();
	}

	la::avdecc::protocol::ProtocolInterface& getProtocolInterface() noexcept
	{
		return *_pi;
	}

private:
	la::avdecc::ExecutorManager::ExecutorWrapper::UniquePointer _ew{ nullptr, nullptr };
	std::unique_ptr<la::avdecc::protocol::ProtocolInterfacePcap> _pi{ nullptr };
};
} // namespace

TEST_F(INTEGRATION_ProtocolInterfacePCap_F, VuDelegate)
{
	// Using MvuAecpdu as class for the tests, so we don't have to design a new VendorUnique class

	static auto selfCommandReceivedPromise = std::promise<void>{};
	static auto vuCreated = false;

	class VuDelegate : public la::avdecc::protocol::ProtocolInterface::VendorUniqueDelegate
	{
	public:
		VuDelegate() noexcept = default;

	private:
		// la::avdecc::protocol::ProtocolInterface::VendorUniqueDelegate overrides
		virtual la::avdecc::protocol::Aecpdu::UniquePointer createAecpdu(la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/, bool const isResponse) noexcept override
		{
			EXPECT_FALSE(vuCreated) << "createAecpdu called twice";
			vuCreated = true;
			return la::avdecc::protocol::MvuAecpdu::create(isResponse);
		}
		virtual bool areHandledByControllerStateMachine(la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/) const noexcept override
		{
			return false;
		}
		virtual void onVuAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/, la::avdecc::protocol::VuAecpdu const& aecpdu) noexcept override
		{
			if (vuCreated)
			{
				auto const& vuAecp = static_cast<la::avdecc::protocol::MvuAecpdu const&>(aecpdu);
				EXPECT_EQ(66u, vuAecp.getCommandType().getValue());
				selfCommandReceivedPromise.set_value();
			}
			else
			{
				EXPECT_TRUE(vuCreated) << "createAecpdu never called";
			}
		}
	};

	auto& pi = getProtocolInterface();
	auto scopedDelegate = std::unique_ptr<VuDelegate, std::function<void(VuDelegate*)>>{ nullptr, [&pi](auto*)
		{
			pi.unregisterVendorUniqueDelegate(la::avdecc::protocol::MvuAecpdu::ProtocolID);
		} };
	auto delegate = VuDelegate{};

	if (!pi.registerVendorUniqueDelegate(la::avdecc::protocol::MvuAecpdu::ProtocolID, &delegate))
	{
		scopedDelegate.reset(&delegate);
	}

	// Try to send using sendAecpCommand (forbidden)
	{
		auto aecpdu = la::avdecc::protocol::MvuAecpdu::create(false);
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::MessageNotSupported, pi.sendAecpCommand(std::move(aecpdu), nullptr));
	}

	{
		auto aecpdu = la::avdecc::protocol::MvuAecpdu::create(false);
		auto& vuAecp = static_cast<la::avdecc::protocol::MvuAecpdu&>(*aecpdu);

		// No need to setup the message base fields for this test, only Mvu one to check it's our message that bounced back
		vuAecp.setCommandType(la::avdecc::protocol::MvuCommandType{ 66u });

		ASSERT_TRUE(!pi.sendAecpMessage(vuAecp));

		// Wait for message to bounce back
		auto status = selfCommandReceivedPromise.get_future().wait_for(std::chrono::seconds(5));
		ASSERT_NE(std::future_status::timeout, status);
	}
}
