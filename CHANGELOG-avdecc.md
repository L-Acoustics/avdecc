# Avdecc Library Changelog
All notable changes to the Avdecc Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

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
