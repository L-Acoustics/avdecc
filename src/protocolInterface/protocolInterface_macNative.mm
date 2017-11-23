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

#include "protocolInterface_macNative.hpp"
#include "protocol/protocolAemAecpdu.hpp"
#include <stdexcept>
#include <functional>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#import <AudioVideoBridging/AudioVideoBridging.h>

#pragma mark Forward declaration of ProtocolInterfaceMacNativeImpl
namespace la
{
	namespace avdecc
	{
		namespace protocol
		{
			class ProtocolInterfaceMacNativeImpl;
		} // namespace protocol
	} // namespace avdecc
} // namespace la

#pragma mark - BridgeInterface Declaration
struct EntityQueues
{
	dispatch_queue_t aecpQueue;
	dispatch_semaphore_t aecpLimiter;
};

@interface BridgeInterface : NSObject <AVB17221EntityDiscoveryDelegate, AVB17221AECPClient, AVB17221ACMPClient>
// Private variables
{
	BOOL _primedDiscovery;
	la::avdecc::protocol::ProtocolInterfaceMacNativeImpl* _protocolInterface;
	
	std::recursive_mutex _lockEntities; /** Lock to protect _localProcessEntities and _registeredAcmpHandlers fields */
	std::unordered_map<la::avdecc::UniqueIdentifier, la::avdecc::entity::LocalEntity&> _localProcessEntities; /** Local entities declared by the running process */
	std::unordered_set<la::avdecc::UniqueIdentifier> _registeredAcmpHandlers; /** List of ACMP handlers that have been registered (that must be removed upon destruction, since there is no removeAllHandlers method) */
	
	std::mutex _lockQueues; /** Lock to protect _entityQueues */
	std::unordered_map<la::avdecc::UniqueIdentifier, EntityQueues> _entityQueues;
	
	std::mutex _lockPending; /** Lock to protect _pendingCommands and _pendingCondVar */
	std::uint32_t _pendingCommands; /** Count of pending (inflight) commands, since there is no way to cancel a command upon destruction (and result block might be called while we already destroyed our objects) */
	std::condition_variable _pendingCondVar;
}

+(BOOL)isSupported;
/** std::string to NSString conversion */
+(NSString*)getNSString:(std::string const&)cString;
/** NSString to std::string conversion */
+(std::string)getStdString:(NSString*)nsString;
+(la::avdecc::networkInterface::MacAddress)getMacAddress:(NSArray*)array;
+(AVB17221Entity*)makeAVB17221Entity:(la::avdecc::entity::Entity const&)entity;
+(la::avdecc::entity::DiscoveredEntity)makeEntity:(AVB17221Entity *)entity;
+(la::avdecc::protocol::AemAecpdu::UniquePointer)makeAemResponse:(AVB17221AECPAEMMessage*)response;
+(la::avdecc::protocol::Acmpdu::UniquePointer)makeAcmpMessage:(AVB17221ACMPMessage*)message;
+(la::avdecc::networkInterface::MacAddress)makeMacAddress:(AVBMACAddress*)macAddress;
+(AVBMACAddress*)makeAVBMacAddress:(la::avdecc::networkInterface::MacAddress const&)macAddress;
+(NSString*)getEntityCapabilities:(AVB17221Entity*)entity;
+(la::avdecc::protocol::ProtocolInterface::Error)getProtocolError:(NSError*)error;

/** Initializer */
-(id)initWithInterfaceName:(NSString*)interfaceName andProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl*)protocolInterface;
/** Deinit method to shutdown every pending operations */
-(void)deinit;
/** Destructor */
-(void)dealloc;

// la::avdecc::protocol::ProtocolInterface bridge methods
// Registration of a local process entity (an entity declared inside this process, not all local computer entities)
-(la::avdecc::protocol::ProtocolInterface::Error)registerLocalEntity:(la::avdecc::entity::LocalEntity&)entity;
// Remove handlers for a local process entity
-(void)removeLocalProcessEntityHandlers:(la::avdecc::entity::LocalEntity const&)entity;
// Unregistration of a local process entity
-(la::avdecc::protocol::ProtocolInterface::Error)unregisterLocalEntity:(la::avdecc::entity::LocalEntity const&)entity;
-(la::avdecc::protocol::ProtocolInterface::Error)enableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity;
-(la::avdecc::protocol::ProtocolInterface::Error)disableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity;
-(BOOL)discoverRemoteEntities;
-(BOOL)discoverRemoteEntity:(la::avdecc::UniqueIdentifier)entityID;
-(la::avdecc::protocol::ProtocolInterface::Error)sendAecpCommand:(la::avdecc::protocol::Aecpdu::UniquePointer&&)aecpdu macAddress:(la::avdecc::networkInterface::MacAddress const&)macAddress handler:(la::avdecc::protocol::ProtocolInterface::AecpCommandResultHandler const&)onResult;
-(la::avdecc::protocol::ProtocolInterface::Error)sendAcmpCommand:(la::avdecc::protocol::Acmpdu::UniquePointer&&)acmpdu handler:(la::avdecc::protocol::ProtocolInterface::AcmpCommandResultHandler const&)onResult;

// Variables
@property (retain) AVBInterface* interface;

@end


#pragma mark - ProtocolInterfaceMacNativeImpl Implementation
namespace la
{
	namespace avdecc
	{
		namespace protocol
		{
			
			class ProtocolInterfaceMacNativeImpl final : public ProtocolInterfaceMacNative
			{
			public:
				// Publicly expose notifyObservers methods so the objC code can use it directly
				using ProtocolInterfaceMacNative::notifyObservers;
				using ProtocolInterfaceMacNative::notifyObserversMethod;
				
				/** Constructor */
				ProtocolInterfaceMacNativeImpl(std::string const& networkInterfaceName)
				: ProtocolInterfaceMacNative(networkInterfaceName)
				{
					// Should not be there if the interface is not supported
					assert(isSupported());

					auto* intName = [BridgeInterface getNSString:networkInterfaceName];
					
#if 0 // We don't need to check for AVB capability/enable on the interface, AVDECC do not require an AVB compatible interface
					// Check the interface is AVB enabled
					if(![AVBInterface isAVBEnabledOnInterfaceNamed:intName])
					{
						throw std::invalid_argument("Interface is not AVB enabled");
					}
					// Check the interface is AVB capable
					if(![AVBInterface isAVBCapableInterfaceNamed:intName])
					{
						throw std::invalid_argument("Interface is not AVB capable");
					}
#endif // 0
					
					// We can now create an AVBInterface from this network interface
					_bridge = [[BridgeInterface alloc] initWithInterfaceName:intName andProtocolInterface:this];
				}
				
				/** Destructor */
				virtual ~ProtocolInterfaceMacNativeImpl() noexcept
				{
					shutdown();
				}
				
				// Deleted compiler auto-generated methods
				ProtocolInterfaceMacNativeImpl(ProtocolInterfaceMacNativeImpl&&) = delete;
				ProtocolInterfaceMacNativeImpl(ProtocolInterfaceMacNativeImpl const&) = delete;
				ProtocolInterfaceMacNativeImpl& operator=(ProtocolInterfaceMacNativeImpl const&) = delete;
				ProtocolInterfaceMacNativeImpl& operator=(ProtocolInterfaceMacNativeImpl&&) = delete;
				
			private:
#pragma mark la::avdecc::protocol::ProtocolInterface overrides
				virtual void shutdown() noexcept override
				{
					if(_bridge != nullptr)
					{
						[_bridge deinit];
#if !__has_feature(objc_arc)
						[_bridge release];
#endif
						_bridge = nullptr;
					}
				}
				
				virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept override
				{
					return [_bridge registerLocalEntity:entity];
				}
				
				virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept override
				{
					return [_bridge unregisterLocalEntity:entity];
				}
				
				virtual Error enableEntityAdvertising(entity::LocalEntity const& entity) noexcept override
				{
					return [_bridge enableEntityAdvertising:entity];
				}
				
				virtual Error disableEntityAdvertising(entity::LocalEntity& entity) noexcept override
				{
					return [_bridge disableEntityAdvertising:entity];
				}
				
				virtual Error discoverRemoteEntities() const noexcept override
				{
					if([_bridge discoverRemoteEntities])
						return ProtocolInterface::Error::NoError;
					return ProtocolInterface::Error::TransportError;
				}
				
				virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override
				{
					if([_bridge discoverRemoteEntity:entityID])
						return ProtocolInterface::Error::NoError;
					return ProtocolInterface::Error::TransportError;
				}
				
				virtual Error sendAdpMessage(Adpdu::UniquePointer&& adpdu) const noexcept override
				{
					return Error::MessageNotSupported;
				}

				virtual Error sendAecpMessage(Aecpdu::UniquePointer&& aecpdu) const noexcept override
				{
					return Error::MessageNotSupported;
				}

				virtual Error sendAcmpMessage(Acmpdu::UniquePointer&& acmpdu) const noexcept override
				{
					return Error::MessageNotSupported;
				}

				virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& macAddress, AecpCommandResultHandler const& onResult) const noexcept override
				{
					return [_bridge sendAecpCommand:std::move(aecpdu) macAddress:macAddress handler:onResult];
				}
				
				virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& /*macAddress*/) const noexcept override
				{
					assert(false && "TBD: To be implemented");
					return ProtocolInterface::Error::InternalError;
				}
				
				virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override
				{
					return [_bridge sendAcmpCommand:std::move(acmpdu) handler:onResult];
				}
				
				virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override
				{
					assert(false && "TBD: To be implemented");
					return ProtocolInterface::Error::InternalError;
				}
				
			private:
#pragma mark Private variables
				BridgeInterface* _bridge{ nullptr };
			};
			
			ProtocolInterfaceMacNative::ProtocolInterfaceMacNative(std::string const& networkInterfaceName)
			: ProtocolInterface(networkInterfaceName)
			{
			}
			
			ProtocolInterface::UniquePointer ProtocolInterfaceMacNative::create(std::string const& networkInterfaceName)
			{
				return std::make_unique<ProtocolInterfaceMacNativeImpl>(networkInterfaceName);
			}
			
			bool ProtocolInterfaceMacNative::isSupported() noexcept
			{
				return [BridgeInterface isSupported];
			}

		} // namespace protocol
	} // namespace avdecc
} // namespace la

#pragma mark - BridgeInterface Implementation
@implementation BridgeInterface

+(BOOL)isSupported
{
	if ([NSProcessInfo instancesRespondToSelector:@selector(isOperatingSystemAtLeastVersion:)])
	{
		// Minimum required version is macOS 10.11.0 (El Capitan)
		return [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:{10, 11, 0}];
	}

	return FALSE;
}

/** std::string to NSString conversion */
+(NSString*)getNSString:(std::string const&)cString
{
	//return [NSString stringWithCString:cString.c_str() encoding:NSWindowsCP1252StringEncoding];
	return [NSString stringWithCString:cString.c_str() encoding:NSUTF8StringEncoding];
}

/** NSString to std::string conversion */
+(std::string)getStdString:(NSString*)nsString
{
	//return std::string([nsString cStringUsingEncoding:NSWindowsCP1252StringEncoding]);
	return std::string([nsString cStringUsingEncoding:NSUTF8StringEncoding]);
}

+(la::avdecc::networkInterface::MacAddress)getMacAddress:(NSArray*)array
{
	la::avdecc::networkInterface::MacAddress mac;

	if (array.count > 0)
	{
		auto data = [(AVBMACAddress*)[array objectAtIndex:0] dataRepresentation];
		if (data.length == mac.size())
		{
			auto const* dataPtr = static_cast<decltype(mac)::value_type const*>(data.bytes);
			for (auto& v : mac)
			{
				v = *dataPtr;
				++dataPtr;
			}
		}
	}
	return mac;
}

+(AVB17221Entity*)makeAVB17221Entity:(la::avdecc::entity::Entity const&)entity
{
	auto e = [[AVB17221Entity alloc] init];

	e.entityID = entity.getEntityID();
	e.entityModelID = entity.getVendorEntityModelID();
	e.entityCapabilities = static_cast<AVB17221ADPEntityCapabilities>(entity.getEntityCapabilities());
	e.talkerStreamSources = entity.getTalkerStreamSources();
	e.talkerCapabilities = static_cast<AVB17221ADPTalkerCapabilities>(entity.getTalkerCapabilities());
	e.listenerStreamSinks = entity.getListenerStreamSinks();
	e.listenerCapabilities = static_cast<AVB17221ADPListenerCapabilities>(entity.getListenerCapabilities());
	e.controllerCapabilities = static_cast<AVB17221ADPControllerCapabilities>(entity.getControllerCapabilities());
	e.identifyControlIndex = entity.getIdentifyControlIndex();
	//e.interfaceIndex = entity.getInterfaceIndex();
	e.associationID = entity.getAssociationID();

	e.gPTPGrandmasterID = entity.getGptpGrandmasterID();
	e.gPTPDomainNumber = entity.getGptpDomainNumber();

	e.timeToLive = entity.getValidTime() * 2u;
	e.availableIndex = 0;
	return e;
}

+(la::avdecc::entity::DiscoveredEntity)makeEntity:(AVB17221Entity *)entity
{
	return {entity.entityID, [BridgeInterface getMacAddress:entity.macAddresses], static_cast<std::uint8_t>(entity.timeToLive / 2u), entity.entityModelID
		, static_cast<la::avdecc::entity::EntityCapabilities>(entity.entityCapabilities)
		, entity.talkerStreamSources, static_cast<la::avdecc::entity::TalkerCapabilities>(entity.talkerCapabilities)
		, entity.listenerStreamSinks, static_cast<la::avdecc::entity::ListenerCapabilities>(entity.listenerCapabilities)
		, static_cast<la::avdecc::entity::ControllerCapabilities>(entity.controllerCapabilities)
		, entity.availableIndex, entity.gPTPGrandmasterID, entity.gPTPDomainNumber
		, entity.identifyControlIndex, entity.interfaceIndex, entity.associationID
	};
}

+(la::avdecc::protocol::AemAecpdu::UniquePointer)makeAemResponse:(AVB17221AECPAEMMessage*)response
{
	auto aemAecpdu = la::avdecc::protocol::AemAecpdu::create();
	auto& aem = static_cast<la::avdecc::protocol::AemAecpdu&>(*aemAecpdu);

	// Set Ether2 fields
#pragma message("TBD: Find a way to retrieve these information")
	//aem.setSrcAddress();
	//aem.setDestAddress();

	// Set AECP fields
	aem.setMessageType(la::avdecc::protocol::AecpMessageType::AemResponse);
	aem.setStatus(la::avdecc::protocol::AecpStatus(response.status));
	aem.setTargetEntityID(response.targetEntityID);
	aem.setControllerEntityID(response.controllerEntityID);
	aem.setSequenceID(response.sequenceID);

	// Set AEM fields
	aem.setUnsolicited(response.isUnsolicited);
	aem.setCommandType(la::avdecc::protocol::AemCommandType(response.commandType));
	if (response.commandSpecificData.length != 0)
		aem.setCommandSpecificData(response.commandSpecificData.bytes, response.commandSpecificData.length);

	return aemAecpdu;
}

+(la::avdecc::protocol::Acmpdu::UniquePointer)makeAcmpMessage:(AVB17221ACMPMessage*)message
{
	auto acmpdu = la::avdecc::protocol::Acmpdu::create();
	auto& acmp = static_cast<la::avdecc::protocol::Acmpdu&>(*acmpdu);

	// Set Ether2 fields
#pragma message("TBD: Find a way to retrieve these information")
	//aem.setSrcAddress();
	//aem.setDestAddress();

	// Set ACMP fields
	acmp.setMessageType(la::avdecc::protocol::AcmpMessageType(message.messageType));
	acmp.setStatus(la::avdecc::protocol::AcmpStatus(message.status));
	acmp.setStreamID(message.streamID);
	acmp.setControllerEntityID(message.controllerEntityID);
	acmp.setTalkerEntityID(message.talkerEntityID);
	acmp.setListenerEntityID(message.listenerEntityID);
	acmp.setTalkerUniqueID(message.talkerUniqueID);
	acmp.setListenerUniqueID(message.listenerUniqueID);
	acmp.setStreamDestAddress([BridgeInterface makeMacAddress:message.destinationMAC]);
	acmp.setConnectionCount(message.connectionCount);
	acmp.setSequenceID(message.sequenceID);
	acmp.setFlags(la::avdecc::entity::ConnectionFlags(message.flags));
	acmp.setStreamVlanID(message.vlanID);

	return acmpdu;
}

+(la::avdecc::networkInterface::MacAddress)makeMacAddress:(AVBMACAddress*)macAddress
{
	la::avdecc::networkInterface::MacAddress mac;
	auto const* data = [macAddress dataRepresentation];
	auto const bufferSize = mac.size() * sizeof(la::avdecc::networkInterface::MacAddress::value_type);

	if (data.length == bufferSize)
		memcpy(mac.data(), data.bytes, bufferSize);

	return mac;
}

+(AVBMACAddress*)makeAVBMacAddress:(la::avdecc::networkInterface::MacAddress const&)macAddress
{
	auto* mac = [[AVBMACAddress alloc] initWithBytes:macAddress.data()];
#if !__has_feature(objc_arc)
	[mac autorelease];
#endif
	return mac;
}

+(NSString*)getEntityCapabilities:(AVB17221Entity*)entity
{
	return [NSString stringWithFormat:@"%@ %@ %@"
					, (entity.talkerCapabilities & AVB17221ADPTalkerCapabilitiesImplemented)?@"Talker":@""
					, (entity.listenerCapabilities & AVB17221ADPListenerCapabilitiesImplemented)?@"Listener":@""
					, (entity.controllerCapabilities & AVB17221ADPControllerCapabilitiesImplemented)?@"Controller":@""
					];
}

+(la::avdecc::protocol::ProtocolInterface::Error)getProtocolError:(NSError*)error
{
	if ([[error domain] isEqualToString:AVBErrorDomain])
	{
		auto const code = IOReturn(error.code);
		switch (code)
		{
			case kIOReturnTimeout:
				return la::avdecc::protocol::ProtocolInterface::Error::Timeout;
			case kIOReturnExclusiveAccess:
				return la::avdecc::protocol::ProtocolInterface::Error::DuplicateLocalEntityID;
			case kIOReturnNotFound:
				return la::avdecc::protocol::ProtocolInterface::Error::UnknownLocalEntity;
			default:
				NSLog(@"Not handled IOReturn error code: %x\n", code);
				assert(false && "Not handled error code");
				return la::avdecc::protocol::ProtocolInterface::Error::TransportError;
		}
	}

	return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
}

-(void)startAsyncOperation
{
	std::lock_guard<decltype(_lockPending)> const lg(_lockPending);
	_pendingCommands++;
}

-(void)stopAsyncOperation
{
	{
		std::lock_guard<decltype(_lockPending)> const lg(_lockPending);
		assert(_pendingCommands > 0 && "Trying to stop async operation, but there is no pending operation");
		_pendingCommands--;
	}
	_pendingCondVar.notify_all();
}

-(void)waitAsyncOperations
{
	// Wait for all remaining async operations to complete
	std::unique_lock<decltype(_lockPending)> sync_lg(_lockPending);
	_pendingCondVar.wait(sync_lg, [self]
	{
		return _pendingCommands == 0;
	});
	assert(_pendingCommands == 0 && "Waited for pending operations to complete, but there is some remaining one!");
}

/** Initializer */
-(id)initWithInterfaceName:(NSString*)interfaceName andProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl*)protocolInterface
{
	self = [super init];
	if(self)
	{
		_primedDiscovery = FALSE;
		_protocolInterface = protocolInterface;
		_pendingCommands = 0u;
		self.interface = [[AVBEthernetInterface alloc] initWithInterfaceName:interfaceName];
		self.interface.entityDiscovery.discoveryDelegate = self;
	}
	return self;
}

/** Deinit method to shutdown every pending operations */
-(void)deinit
{
	// Remove discovery delegate
	self.interface.entityDiscovery.discoveryDelegate = nil;

	decltype(_localProcessEntities) localProcessEntities;
	decltype(_registeredAcmpHandlers) registeredAcmpHandlers;
	// Move internal lists to temporary objects while locking, so we can safely cleanup outside of the lock
	{
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
		localProcessEntities = std::move(_localProcessEntities);
		registeredAcmpHandlers = std::move(_registeredAcmpHandlers);
	}

	// Remove LocalEntities that were not removed
	for (auto const& entityKV : localProcessEntities)
	{
		auto const& entity = entityKV.second;
		// Remove remaining handlers
		[self removeLocalProcessEntityHandlers:entity];
		// Disable advertising
		[self disableEntityAdvertising:entity];
	}

	// Remove ACMP handlers that were not removed
	for (auto const entityID : registeredAcmpHandlers)
	{
		[self deinitEntity:entityID];
	}

	// Wait for remaining pending operations
	[self waitAsyncOperations];

	// Release 1722.1 interface
	self.interface = nil;
}

/** Destructor */
-(void)dealloc
{
	[self deinit];
#if !__has_feature(objc_arc)
	[super dealloc];
#endif
}

#pragma mark la::avdecc::protocol::ProtocolInterface bridge methods
// Registration of a local process entity (an entity declared inside this process, not all local computer entities)
-(la::avdecc::protocol::ProtocolInterface::Error)registerLocalEntity:(la::avdecc::entity::LocalEntity&)entity
{
	// Lock entities now, so we don't get interrupted during registration
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);

	auto const entityID = entity.getEntityID();

	// Entity is controller capable
	if (la::avdecc::hasFlag(entity.getControllerCapabilities(), la::avdecc::entity::ControllerCapabilities::Implemented))
	{
		// Set a handler to monitor AECP Response messages, we are interested in unsolicited notifications
		[self.interface.aecp setResponseHandler:self forControllerEntityID:entityID];
	}
	// Other types are not supported right now
	else
	{
		return la::avdecc::protocol::ProtocolInterface::Error::InvalidEntityType;
	}

	// Add the entity to our cache of local entities declared by the running program
	_localProcessEntities.insert(decltype(_localProcessEntities)::value_type(entityID, entity));

	// Notify observers (creating a DiscoveredEntity from a LocalEntity)
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, entity);
	
		return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

// Remove handlers for a local process entity
-(void)removeLocalProcessEntityHandlers:(la::avdecc::entity::LocalEntity const&)entity
{
	auto const entityID = entity.getEntityID();

	// Entity is controller capable
	if (la::avdecc::hasFlag(entity.getControllerCapabilities(), la::avdecc::entity::ControllerCapabilities::Implemented))
	{
		// Remove handler
		[self.interface.aecp removeResponseHandlerForControllerEntityID:entityID];
	}
}

// Unregistration of a local process entity
-(la::avdecc::protocol::ProtocolInterface::Error)unregisterLocalEntity:(la::avdecc::entity::LocalEntity const&)entity
{
	auto const entityID = entity.getEntityID();

	// Remove handlers
	[self removeLocalProcessEntityHandlers:entity];

	// Disable advertising
	[self disableEntityAdvertising:entity];

	// Lock entities now that we have removed the handlers
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
	// Remove the entity from our cache of local entities declared by the running program
	_localProcessEntities.erase(entityID);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOffline, entityID);

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

-(la::avdecc::protocol::ProtocolInterface::Error)enableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity
{
	NSError* error{nullptr};
	
	[self.interface.entityDiscovery addLocalEntity:[BridgeInterface makeAVB17221Entity:entity] error:&error];
	if (error != nullptr)
		return [BridgeInterface getProtocolError:error];

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

-(la::avdecc::protocol::ProtocolInterface::Error)disableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity
{
	NSError* error{nullptr};

	[self.interface.entityDiscovery removeLocalEntity:entity.getEntityID() error:&error];
	if (error != nullptr)
		return [BridgeInterface getProtocolError:error];

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

-(BOOL)discoverRemoteEntities
{
	if(!_primedDiscovery)
	{
		[self.interface.entityDiscovery primeIterators];
		_primedDiscovery = TRUE;
		return TRUE;
	}
	return [self.interface.entityDiscovery discoverEntities];
}

-(BOOL)discoverRemoteEntity:(la::avdecc::UniqueIdentifier)entityID
{
	if(!_primedDiscovery)
	{
		[self.interface.entityDiscovery primeIterators];
		_primedDiscovery = TRUE;
		return TRUE;
	}
	return [self.interface.entityDiscovery discoverEntity:entityID];
}

-(la::avdecc::protocol::ProtocolInterface::Error)sendAecpCommand:(la::avdecc::protocol::Aecpdu::UniquePointer&&)aecpdu macAddress:(la::avdecc::networkInterface::MacAddress const&)macAddress handler:(la::avdecc::protocol::ProtocolInterface::AecpCommandResultHandler const&)onResult
{
	auto const macAddr = macAddress; // Make a copy of the macAddress so it can safely be used inside the objC block
	auto const resultHandler = onResult; // Make a copy of the handler so it can safely be used inside the objC block

	auto const messageType = aecpdu->getMessageType();
	if(messageType == la::avdecc::protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<la::avdecc::protocol::AemAecpdu const&>(*aecpdu);
		auto const message = [AVB17221AECPAEMMessage commandMessage];
		// Set Aem specific fields
		message.unsolicited = FALSE;
		message.controllerRequest = FALSE;
		message.commandType = static_cast<AVB17221AEMCommandType>(aem.getCommandType().getValue());
		auto const payloadInfo = aem.getPayload();
		auto const* const payload = payloadInfo.first;
		if (payload != nullptr)
		{
			message.commandSpecificData = [NSData dataWithBytes:payload length:payloadInfo.second];
		}
		// Set common fields
		message.status = AVB17221AECPStatusSuccess;
		message.targetEntityID = aem.getTargetEntityID();
		message.controllerEntityID = aem.getControllerEntityID();
		// No need to set the sequenceID field, it's handled by Apple's framework

		decltype(EntityQueues::aecpQueue) queue;
		// Only take the lock while searching for the queue, we want to release it before invoking dispath_async to prevent a deadlock
		{
			std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
			auto eqIt = _entityQueues.find(aem.getTargetEntityID());
			if (eqIt == _entityQueues.end())
			{
				assert(false && "Should not happen");
				return la::avdecc::protocol::ProtocolInterface::Error::UnknownRemoteEntity;
			}
			queue = eqIt->second.aecpQueue;
		}

		dispatch_async(queue, ^{
			dispatch_semaphore_t limiter;
			{
				std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
				auto eqIt = _entityQueues.find(message.targetEntityID);
				if (eqIt == _entityQueues.end())
				{
					// Entity no longer registered, ignore this command and return
					return;
				}
				limiter = eqIt->second.aecpLimiter; // We can store the limiter here, we know the queue and semaphore exists until all scheduled blocks on this queue are finished (thanks to the dispatch_sync call)
			}

			// Take a semaphore count to limit the inflight commands
			dispatch_semaphore_wait(limiter, DISPATCH_TIME_FOREVER);

			[self startAsyncOperation];
			if ([self.interface.aecp sendCommand:message toMACAddress:[BridgeInterface makeAVBMacAddress:macAddr] completionHandler:^(NSError* error, AVB17221AECPMessage* message)
					 {
						 if (kIOReturnSuccess == (IOReturn)error.code)
						 {
							 assert([message messageType] == AVB17221AECPMessageTypeAEMResponse && "AECP Response to our AEM Command is NOT an AEM Response!");
							 auto aem = [BridgeInterface makeAemResponse:static_cast<AVB17221AECPAEMMessage*>(message)];
							 resultHandler(aem.get(), la::avdecc::protocol::ProtocolInterface::Error::NoError);
						 }
						 else
						 {
							 resultHandler(nullptr, [BridgeInterface getProtocolError:error]);
						 }
						 [self stopAsyncOperation];
						 // Signal the semaphore so we can process another command
						 dispatch_semaphore_signal(limiter);
					 }] == NO)
			{
				// Failed to send the message
				NSLog(@"Failed to send AECP message");
				[self stopAsyncOperation];
				resultHandler(nullptr, la::avdecc::protocol::ProtocolInterface::Error::TransportError);
				// Signal the semaphore now, the aecp sendCommand handler won't fire
				dispatch_semaphore_signal(limiter);
			}
		});
	}
	else
	{
		assert(false && "Not supported AECP message type");
		return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
	}
	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

-(la::avdecc::protocol::ProtocolInterface::Error)sendAcmpCommand:(la::avdecc::protocol::Acmpdu::UniquePointer&&)acmpdu handler:(la::avdecc::protocol::ProtocolInterface::AcmpCommandResultHandler const&)onResult
{
	auto const resultHandler = onResult; // Make a copy of the handler so it can safely be used inside the objC block

	auto const& acmp = static_cast<la::avdecc::protocol::Acmpdu const&>(*acmpdu);
	auto const message = [[AVB17221ACMPMessage alloc] init];
#if !__has_feature(objc_arc)
	[message autorelease];
#endif
	// Set Acmp fields
	message.messageType = static_cast<AVB17221ACMPMessageType>(acmp.getMessageType().getValue());
	message.status = AVB17221ACMPStatusSuccess;
	message.streamID = acmp.getStreamID();
	message.controllerEntityID = acmp.getControllerEntityID();
	message.talkerEntityID = acmp.getTalkerEntityID();
	message.talkerUniqueID = acmp.getTalkerUniqueID();
	message.listenerEntityID = acmp.getListenerEntityID();
	message.listenerUniqueID = acmp.getListenerUniqueID();
	message.destinationMAC = [BridgeInterface makeAVBMacAddress:acmp.getStreamDestAddress()];
	message.connectionCount = acmp.getConnectionCount();
	// No need to set the sequenceID field, it's handled by Apple's framework
	message.flags = static_cast<AVB17221ACMPFlags>(acmp.getFlags());
	message.vlanID = acmp.getStreamVlanID();

	[self startAsyncOperation];
	if ([self.interface.acmp sendACMPCommandMessage:message completionHandler:^(NSError* error, AVB17221ACMPMessage* message)
		{
			if (kIOReturnSuccess == (IOReturn)error.code)
			{
				auto acmp = [BridgeInterface makeAcmpMessage:message];
				resultHandler(acmp.get(), la::avdecc::protocol::ProtocolInterface::Error::NoError);
			}
			else
			{
				resultHandler(nullptr, [BridgeInterface getProtocolError:error]);
			}
			[self stopAsyncOperation];
		}] == NO)
	{
		// Failed to send the message
		NSLog(@"Failed to send ACMP message");
		[self stopAsyncOperation];
		return la::avdecc::protocol::ProtocolInterface::Error::TransportError;
	}
	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

#pragma mark AVB17221EntityDiscoveryDelegate delegate
-(void)initEntity:(la::avdecc::UniqueIdentifier)entityID
{
	// Register ACMP sniffing handler for this entity
	if ([self.interface.acmp setHandler:self forEntityID:entityID])
	{
		// Register the entity for handler removal upon shutdown
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
		_registeredAcmpHandlers.insert(entityID);
	}
	
	// Create queues and semaphore
	{
		std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
		EntityQueues eq;
		eq.aecpQueue = dispatch_queue_create([[NSString stringWithFormat:@"%@.0x%016llx.aecp", [self className], entityID] UTF8String], 0);
		eq.aecpLimiter = dispatch_semaphore_create(la::avdecc::protocol::Aecpdu::DefaultMaxInflightCommands);
		_entityQueues[entityID] = std::move(eq);
	}
}

-(void)deinitEntity:(la::avdecc::UniqueIdentifier)entityID
{
	{
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
		
		// Unregister ACMP handler
		_registeredAcmpHandlers.erase(entityID);
		[self.interface.acmp removeHandlerForEntityID:entityID];
	}

	// Remove the EntityQueues structure from the map under the lock (if found), then clean it without the lock (to prevent deadlock)
	EntityQueues eq;
	bool foundEq{false};
	{
		std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
		auto eqIt = _entityQueues.find(entityID);
		if (eqIt != _entityQueues.end())
		{
			eq = eqIt->second; // Copy the structure now, we will remove it from the map next line (if ARC is enabled, a ref count will be taken. If not we still haven't call dispatch_release so the object is still valid)
			//NSLog(@"Queue retain count before sync: %ld", eq.aecpQueue.retainCount);
			//NSLog(@"Limiter retain count before sync: %ld", eq.aecpLimiter.retainCount);

			// Remove from the map to indicate it's being removed
			_entityQueues.erase(entityID);
			
			// We found it, we can clean it up
			foundEq = true;
		}
	}
	
	// Found a matching EntityQueues for the entity, time to clean it up
	if (foundEq)
	{
		// Synchronize the queue using an empty block
		dispatch_sync(eq.aecpQueue, ^{
			//NSLog(@"Queue retain count: %ld", eq.aecpQueue.retainCount);
			//NSLog(@"Limiter retain count: %ld", eq.aecpLimiter.retainCount);
		});
#if !__has_feature(objc_arc)
		// Release the objects
		dispatch_release(eq.aecpQueue);
		dispatch_release(eq.aecpLimiter);
#endif
	}
}

// Notification of an arriving local computer entity
-(void)didAddLocalEntity:(AVB17221Entity *)newEntity on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	[self initEntity:newEntity.entityID];

	// Notify observers
	auto e = [BridgeInterface makeEntity:newEntity];
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, e);
}

// Notification of a departing local computer entity
-(void)didRemoveLocalEntity:(AVB17221Entity *)oldEntity on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	[self deinitEntity:oldEntity.entityID];

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOffline, oldEntity.entityID);
}

-(void)didRediscoverLocalEntity:(AVB17221Entity *)entity on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	// Check if Entity already in the list
	{
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
		if (_registeredAcmpHandlers.find(entity.entityID) == _registeredAcmpHandlers.end())
		{
			assert(false && "didRediscoverLocalEntity: Entity not registered... I thought Rediscover was called when an entity announces itself again without any change in it's ADP info... Maybe simply call didAddLocalEntity");
			return;
		}
	}
	// Nothing to do, entity has already been detected
}

-(void)didUpdateLocalEntity:(AVB17221Entity *)entity changedProperties:(AVB17221EntityPropertyChanged)changedProperties on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	constexpr NSUInteger ignoreChangeMask = 0xFFFFFFFF & ~(AVB17221EntityPropertyChangedTimeToLive | AVB17221EntityPropertyChangedAvailableIndex);
	// If changes are only for flags we want to ignore, return
	if ((changedProperties & ignoreChangeMask) == 0)
		return;

	auto e = [BridgeInterface makeEntity:entity];

	// If a change occured in a forbidden flag, simulate offline/online for this entity
	if ((changedProperties & kAVB17221EntityPropertyChangedShouldntChangeMask) != 0)
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOffline, e.getEntityID());
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, e);
	}
	else
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityUpdated, e);
	}
}

-(void)didAddRemoteEntity:(AVB17221Entity *)newEntity on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	[self initEntity:newEntity.entityID];

	// Notify observers
	auto e = [BridgeInterface makeEntity:newEntity];
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOnline, e);
}

-(void)didRemoveRemoteEntity:(AVB17221Entity *)oldEntity on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	[self deinitEntity:oldEntity.entityID];

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOffline, oldEntity.entityID);
}

-(void)didRediscoverRemoteEntity:(AVB17221Entity *)entity on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	// Check if Entity already in the list
	{
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
		if (_registeredAcmpHandlers.find(entity.entityID) == _registeredAcmpHandlers.end())
		{
			assert(false && "didRediscoverRemoteEntity: Entity not registered... I thought Rediscover was called when an entity announces itself again without any change in it's ADP info... Maybe simply call didAddRemoteEntity");
			return;
		}
	}
	// Nothing to do, entity has already been detected
}

-(void)didUpdateRemoteEntity:(AVB17221Entity *)entity changedProperties:(AVB17221EntityPropertyChanged)changedProperties on17221EntityDiscovery:(AVB17221EntityDiscovery *)entityDiscovery
{
	constexpr NSUInteger ignoreChangeMask = 0xFFFFFFFF & ~(AVB17221EntityPropertyChangedTimeToLive | AVB17221EntityPropertyChangedAvailableIndex);
	// If changes are only for flags we want to ignore, return
	if ((changedProperties & ignoreChangeMask) == 0)
		return;

	auto e = [BridgeInterface makeEntity:entity];

	// If a change occured in a forbidden flag, simulate offline/online for this entity
	if ((changedProperties & kAVB17221EntityPropertyChangedShouldntChangeMask) != 0)
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOffline, e.getEntityID());
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOnline, e);
	}
	else
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityUpdated, e);
	}
}

#pragma mark AVB17221AECPClient delegate
-(BOOL)AECPDidReceiveCommand:(AVB17221AECPMessage *)message onInterface:(AVB17221AECPInterface *)anInterface
{
	return NO;
}

-(BOOL)AECPDidReceiveResponse:(AVB17221AECPMessage *)message onInterface:(AVB17221AECPInterface *)anInterface
{
	// This handler is called for all AECP messages to our ControllerID, even the messages that are solicited responses and which will be handled by the block of aecp.sendCommand() method

	// Search our controller entity, which should be found!
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);
	auto it = _localProcessEntities.find([message controllerEntityID]);
	if (it == _localProcessEntities.end())
		return NO;
	auto const& entity = it->second;

	switch ([message messageType])
	{
		case AVB17221AECPMessageTypeAEMResponse:
		{
			auto aemMessage = static_cast<AVB17221AECPAEMMessage*>(message);
			// We are only interested in unsolicited messages, expected responses are handled in objC block result
			if (aemMessage.unsolicited)
			{
				auto aecpdu = [BridgeInterface makeAemResponse:aemMessage];
				_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAecpUnsolicitedResponse, entity, *aecpdu);
				return YES;
			}
			break;
		}
		default:
			break;
	}
	return NO;
}

#pragma mark AVB17221ACMPClient delegate
-(BOOL)ACMPDidReceiveCommand:(AVB17221ACMPMessage *)message onInterface:(AVB17221ACMPInterface *)anInterface
{
	// This handler is called for all ACMP messages, even the messages that are sent by ourself

	BOOL processedBySomeone{ NO };
	auto acmpdu = [BridgeInterface makeAcmpMessage:message];

	// Dispatch sniffed message to registered controllers
	// Lock entities
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);

	for (auto& localEntityIt : _localProcessEntities)
	{
		auto const& entity = localEntityIt.second;

		// Entity is controller capable
		if (la::avdecc::hasFlag(entity.getControllerCapabilities(), la::avdecc::entity::ControllerCapabilities::Implemented))
		{
			_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAcmpSniffedCommand, entity, *acmpdu);
			processedBySomeone = YES;
		}
	}

	return processedBySomeone;
}

-(BOOL)ACMPDidReceiveResponse:(AVB17221ACMPMessage *)message onInterface:(AVB17221ACMPInterface *)anInterface
{
	// This handler is called for all ACMP messages, even the messages that are expected responses and which will be handled by the block of acmp.sendACMPCommandMessage() method

	BOOL processedBySomeone{ NO };
	auto acmpdu = [BridgeInterface makeAcmpMessage:message];

	// Dispatch sniffed message to registered controllers
	// Lock entities
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities);

	for (auto& localEntityIt : _localProcessEntities)
	{
		auto const& entity = localEntityIt.second;

		// Entity is controller capable
		if (la::avdecc::hasFlag(entity.getControllerCapabilities(), la::avdecc::entity::ControllerCapabilities::Implemented))
		{
			_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAcmpSniffedResponse, entity, *acmpdu);
			processedBySomeone = YES;
		}
	}

	return processedBySomeone;
}

@end

