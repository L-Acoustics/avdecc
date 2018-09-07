# Avdecc Library Changelog
All notable changes to the Avdecc Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Support for Memory Operations (Contributed by Florian Harmuth)
- Protocol and ProtocolInterface classes exposed in the public API
- Removed InterfaceIndex field from ProtocolInterface (not its place)

### Changed
- SupportedProtocolInterfaceTypes is now la::avdecc::EnumBitfield

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
