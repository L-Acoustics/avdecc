# Avdecc Library Changelog
All notable changes to the Avdecc Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Support for JACK_INPUT/JACK_OUTPUT descriptors
- Support for CONTROL descriptors at AUDIO_UNIT, JACK, STREAM_PORT levels
- [Support for CONTROL_SELECTOR type for CONTROL descriptors](https://github.com/L-Acoustics/avdecc/issues/128)
- *numberOfValues* field in the CONTROL descriptor
- Support for float special values in json dumps (ie. NaN, -inf, +inf)
- Support for UTF8 file paths

### Changed
- la::avdecc::entity::controller::Delegate is now virtual pure, but a new derivated visitor (with all default implementation) has been added: la::avdecc::entity::controller::DefaultedDelegate
- [Executor name can be provided when creating an EndStation](https://github.com/L-Acoustics/avdecc/issues/132)
- *entity::model::validateControlValues* now returns an enum value as well as an error message

### Fixed
- Crash when unpacking vendor specific control values

## [3.4.1] - 2023-01-11

## [3.4.0] - 2023-01-05
### Added
- Checking validity of EntityModel given to a ControllerEntity or AggregateEntity
- Added `sequenceID` parameter to `la::avdecc::entity::controller::Delegate::onAemAecpUnsolicitedReceived`

### Fixed
- EntityModel loader correctly set current active configuration
- Better handling of NOT_SUPPORTED AEM-AECP responses

## [3.3.0] - 2022-11-22
### Added
- A la::avdecc::entity::model::EntityTree can be fed into a LocalEntity so it responds to AEM query commands (currently limited to ENTITY descriptor)
- EndStation::deserializeEntityModelFromJson to unpack a la::avdecc::entity::model::EntityTree from a file
- [Full support for npcap (prefered over winpcap if both installed)](https://github.com/L-Acoustics/avdecc/issues/114)

### Changed
- Reflecting an unhandled AECP command uses NOT_IMPLEMENTED instead of NOT_SUPPORTED

### Fixed
- Crash when loading ANS/AVE file without any configuration descriptor
- Discarding inflight and queued messages when an entity goes offline

## [3.2.4] - 2022-07-08
### Fixed
- Possible crash during uninitialization of the EndStation

## [3.2.3] - 2022-04-22
### Added
- injectRawPacket method to ProtocolInterface

### Changed
- Generated EntityID now follows the new recommendation of IEEE (full MacAddress on MSB)
- Full network message scheduler refactoring

## [3.2.2] - 2022-01-20
### Added
- [Support for Control Array Values Type (Clause 7.3.5.2.3)](https://github.com/L-Acoustics/avdecc/issues/100)
- [Support for REBOOT command](https://github.com/L-Acoustics/avdecc/issues/99)

### Changed
- la::avdecc::entity::model::StreamFormatInfo now uses la::avdecc::entity::model::SamplingRate instead of its own enum

### Fixed
- ANS/AVE loading error when msrp_failure_code is defined to null
- [Control UTF8 Value Type properly saved in json model](https://github.com/L-Acoustics/avdecc/issues/101)

## [3.2.1] - 2021-12-10

## [3.2.0] - 2021-07-21
### Added
- isGroupIdentifier and isLocalIdentifier methods to la::avdecc::UniqueIdentifier
- [setAssociation and getAssociation controller commands](https://github.com/L-Acoustics/avdecc/issues/32)

### Changed
- [Changed msrpFailureCode from uint8 to enum class](https://github.com/L-Acoustics/avdecc/issues/98)

### Fixed
- [Incorrect generated AUDIO_MAP index when loading a virtual entity](https://github.com/L-Acoustics/avdecc/issues/94)
- [Possible stack corruption when receiving response violating the protocol](https://github.com/L-Acoustics/avdecc/issues/97)
- [AssociationID is now always stored as a std::optional<UniqueIdentifier>](https://github.com/L-Acoustics/avdecc/issues/79)

## [3.1.1] - 2021-04-02
### Added
- Validating Control dynamic values (based on static values)

### Changed
- Renamed la::avdecc::entity::model::ControlType to la::avdecc::entity::model::StandardControlType
- la::avdecc::entity::model::ControlType is now an EUI-64

### Fixed
- [Correctly exporting all ControlValues structures](https://github.com/L-Acoustics/avdecc/issues/90)
- Ignoring AECP response from an incorrect source

## [3.1.0] - 2021-01-18
### Added
- [Support for Control Descriptors (only Linear Values are currently supported)](https://github.com/L-Acoustics/avdecc/issues/88)
- [Support for Controller to Entity Identification](https://github.com/L-Acoustics/avdecc/issues/13)

### Changed
- When an observer registers to a ProtocolInterface, it now immediately receives notifications about already known entities (both local and remote)
- ControllerEntity now always sends messages to the same AvbInterface in case of cable redundancy

### Fixed
- [ControllerCapability methods trigger the result handler in case of Protocol exception](https://github.com/L-Acoustics/avdecc/issues/83)
- [Aecpdu::MaximumLength_BigPayloads increased to fill maximum Ethernet frame size (if compile defined is set) allowing up to 184 mappings to be received from a Milan entity](https://github.com/L-Acoustics/avdecc/issues/82)
- [Detection of main AvbInterface loss in cable redundancy](https://github.com/L-Acoustics/avdecc/issues/55)

## [3.0.2] - 2020-09-14
### Added
- Automatic discovery delay is now configurable for a Controller Entity

### Fixed
- [Deadlock when shutting down PCap ProtocolInterface on linux](https://github.com/L-Acoustics/avdecc/issues/81)
- Possible crash (uncaught exception) on macOS when enumerating network interfaces

## [3.0.1] - 2020-05-25
### Changed
- Improved la::avdecc::utils::EnumBitfield class to be fully constexpr

### Fixed
- [macOS 10.14 fails to load libpcap](https://github.com/L-Acoustics/avdecc/issues/78)

## [3.0.0] - 2020-03-10
### Added
- AEM GetConfiguration method
- ProtocolInterface observer events for all received PDUs (low level)
- Delegate class in ProtocolInterface to handle VendorUnique messages
- [ACMP inflight queue](https://github.com/L-Acoustics/avdecc/issues/17)
- [AECP/ACMP throttling](https://github.com/L-Acoustics/avdecc/issues/17)
- Partial support for VendorUnique message in macOS Native ProtocolInterface
- C Bindings library

### Changed
- Controller commands result handler properly return all fields in the protocol
- Removed listenerStream field from entity::model::StreamConnectionState
- Renamed entity::model::StreamConnectionState to entity::model::StreamInputConnectionInfo
- MacOS Native ProtocolInterface is now restricted to macOS Catalina and later

### Fixed
- [Crashes/deadlock when using macOS native](https://github.com/L-Acoustics/avdecc/issues/73)

## [2.11.3] - 2019-11-20

## [2.11.2] - 2019-11-08

## [2.11.1] - 2019-10-02
### Changed
- More control over what is dumped when serializing an entity

## [2.11.0] - 2019-09-10
### Added
- Precompiled headers on windows
- Improved protocol warning messages

## [2.10.0] - 2019-06-24
### Added
- Entity Model Tree definition
- Entity Model dump as readable json file
- Entity Model load from readable json file
- NetworkInterfaceHelper observer to monitor changes in adapters status
- [Controller statistics counters](https://github.com/L-Acoustics/avdecc/issues/41)

### Changed
- NetworkInterfaceHelper _isActive_ field replace with _isEnabled_ and _isConnected_
- _protocol::MvuFeaturesFlags_ TypedDefine moved to _MilanInfoFeaturesFlags_ EnumBitfield

### Fixed
- [NetworkInterfaces Helper fully working on win7 and win10](https://github.com/L-Acoustics/avdecc/issues/59)
- json::Exception being thrown across shared library boundary

## [2.9.2] - 2019-05-20
### Added
- Support for IdentifyNotification in ProtocolInterface and ControllerEntity
- Support for Entity Descriptor counters

## [2.9.1] - 2019-03-13
### Changed
- [Replaced all la::avdecc::enum_traits with la::avdecc::EnumBitfield](https://github.com/L-Acoustics/avdecc/issues/34)
- StreamFormat and SamplingRate now have their own class, instead of just an alias

### Fixed
- macOS Native Protocol Interface correctly handles incoming AECP Commands
- macOS Native Protocol Interface simulates a time out for VENDOR_UNIQUE messages, instead of incorrectly handling them (time out required due to bug in AVBFramework)

### Removed
- GenericAecpdu class, due to potential misusage
- Aecpdu::copy method, no longer necessary

## [2.9.0] - 2019-02-13
### Added
- Support for Milan STREAM_OUTPUT counters

### Changed
- Removed macAddress parameter from sendAecpCommand and sendAecpResponse (automatically getting it from the AECPDU)
- Returning ProtocolInterface::Error::InvalidParameters when trying to enable/disable advertising on a non-existent InterfaceIndex

### Fixed
- Incorrect error returned by EndStation's LocalEntity creation code
- No longer sending ADP DEPARTING message twice when destroying a LocalEntity

## [2.8.0] - 2019-01-23
### Added
- Notification when a controller is being deregistered from unsolicited notifications
- Notification when (some or all) audio mappings are added or removed
- Notification when an entity is locked/unlocked by another controller
- setStreamInputInfo, setStreamOutputInfo, getAsPath
- Milan extended GetStreamInfo
- Code to try to prevent deadlocks
- Watch dog thread

### Changed
- lockEntity/unlockEntity signature and result handler changed to include the locking entity and descriptor type/index
- GetMilanInfo now return a struct instead of individual fields
- Updated GetMilanInfo to match Milan specification

### Fixed
- Incorrect value for AemLockEntityFlags::Unlock flag

## [2.7.2] - 2018-10-30

## [2.7.1] - 2018-10-02
### Added
- [Support for GET_COUNTERS command and unsolicited notifications](https://github.com/L-Acoustics/avdecc/issues/12)
- Support for VendorUnique AECP messages
- Support for 500Hz CRF stream format

### Changed
- Split ALLOW_BIG_AEM_PAYLOADS to ALLOW_SEND_BIG_AECP_PAYLOADS and ALLOW_RECV_BIG_AECP_PAYLOADS

## [2.7.0] - 2018-09-12
### Added
- Support for MemoryObject Operations (Contributed by d&b)
- Protocol and ProtocolInterface classes exposed in the public API
- Removed InterfaceIndex field from ProtocolInterface (not its place)

### Changed
- la::avdecc::protocol::ProtocolInterface::SupportedProtocolInterfaceTypes and la::avdecc::CompileOptions are now la::avdecc::EnumBitfield

### Fixed
- ALLOW_BIG_AEM_PAYLOADS option not properly used in serializer

## [2.6.3] - 2018-08-08

## [2.6.2] - 2018-08-06
### Fixed
- [CRF StreamFormat should be a 0 channel stream, not 1](https://github.com/L-Acoustics/avdecc/issues/28)
- [ControllerStateMachine AECP/ACMP SequenceID starts at 0, not 1](https://github.com/L-Acoustics/avdecc/issues/21)

## [2.6.1] - 2018-07-30
### Added
- [Support for AddressAccess AECP sub-protocol](https://github.com/L-Acoustics/avdecc/issues/20)

## [2.6.0] - 2018-07-17
### Added
- getStreamInputFormat, getStreamOutputFormat, setAudioUnitName, getAudioUnitName, setAvbInterfaceName, getAvbInterfaceName, setClockSourceName, getClockSourceName, getClockSource, setMemoryObjectName, getMemoryObjectName, setAudioClusterName, getAudioClusterName, setClockDomainName, getClockDomainName, getAudioUnitSamplingRate, getVideoClusterSamplingRate, getSensorClusterSamplingRate, setMemoryObjectLength, getMemoryObjectLength
- Notification callbacks for onAudioUnitNameChanged, onAvbInterfaceNameChanged, onClockSourceNameChanged, onMemoryObjectNameChanged, onAudioClusterNameChanged, onClockDomainNameChanged, onMemoryObjectLengthChanged

### Changed
- la::avdecc::Entity::getVendorEntityModelID() renamed getEntityModelID()
- la::avdecc::entity::model::makeVendorEntityModel() renamed makeEntityModelID()
- la::avdecc::entity::model::splitVendorEntityModel() renamed splitEntityModelID()
- Every occurrence of vendorEntityModelID renamed entityModelID
- la::avdecc::UniqueIdentifier is now a complex type with member helper methods
- some methods of la::avdecc::protocol::Adpdu were not declared noexcept
- la::avdecc::entity::model::AvbInterfaceDescriptor::clockIdentify renamed clockIdentity

### Removed
- la::avdecc::entity::model::VendorEntityModel (should be la::avdecc::UniqueIdentifier)

### Fixed
- Getting system current time after mutex has been acquired, not before

## [2.5.3] - 2018-06-18
### Fixed
- Descriptors value initialization for LocalizedStringReference type
- [Possible crash using "macOS native interface"](https://github.com/L-Acoustics/avdecc/issues/7)

## [2.5.2] - 2018-06-15
### Added
- New static method to get adapted compatible formats from listener and talker available ones
- New static methods to build StreamFormat from attributes

## [2.5.0] - 2018-06-01
### Fixed
- Removed noexcept qualifier from all std::function (c++17 compatibility)
- Allowing AEM payloads exceeding maximum protocol value (new compilation flag: ALLOW_BIG_AEM_PAYLOADS). Required for Q6 Core devices.

## [2.4.0] - 2018-05-15
### Added
- Global API to retrieve the compilation options of the library
- CMake compilation option to use custom std::any

### Fixed
- Properly handling AvailableIndex going backwards in ADP messages (fast entity powercycle)
- Possible deadlock when using macOS native protocol interface
- Re-enabled AVnu Alliance Network Redundancy support

## [2.3.0] - 2018-03-30
### Added
- Support for the following read descriptors: AudioUnit, Jack, AvbInterface, ClockSource, MemoryObject, StreamPort, ExternalPort, InternalPort, AudioCluster, AudioMap, ClockDomain
- Cole Peterson's redundant streams association detection
- Support for Talker Stream Connections
- Method to forcefully remove a connection from a talker's StreamOutput (DisconnectTX spoofing)
- Support for GetAvbInfo command and unsolicited notifications
- getSampleSize and getSampleBitDepth to StreamFormatInfo

### Changed
- ACMP stream identification refactoring (new type describing the couple EntityID/StreamIndex)

### Fixed
- 50 msec delay before sending a new AECP message using pcap protocol interface
- Logger observer not removed during unregisterObserver

## [2.2.0] - 2017-12-01
### Added
- Support for setConfiguration command
- [Partial support for SET_CONFIGURATION unsolicited notification](https://github.com/L-Acoustics/avdecc/issues/3)
- Virtual ProtocolInterface, allowing better unit testing

### Fixed
- Possible crash during shutdown
- IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES flag not properly working for Motu devices

## [2.1.0] - 2017-11-22
### Changed
- Acquire/Release controller commands (and handlers) now support descriptor type/index

## [2.0.10] - 2017-10-27
### Fixed
- [Crash when running on macOS < 10.11 using macOS native protocol interface](https://github.com/L-Acoustics/avdecc/issues/1)

## [2.0.0] - 2017-10-10
### Changed
- Full rework of the low level data structures

## [1.0.0] - 2016-01-15
### Added
- Support for minimal audio related 1722.1 messages
