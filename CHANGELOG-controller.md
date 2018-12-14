# Avdecc Controller Library Changelog
All notable changes to the Avdecc Controller Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Better detection of non IEEE1722.1 compliant entities, and misbehaving entities (sending correctly built messages but with incoherent values)
- Detecting when the controller is being deregistered from unsolicited notifications (or if the entity does not support it)
- Support for lock/unlock commands
- Support for AsPath (query during enumeration, and change notification)
- Support for setStreamInfo command
- [Retrieving entity current lock state upon enumeration](https://github.com/L-Acoustics/avdecc/issues/26)
- AVB Interface link status (when available)
- New enum value for AcquireState and LockState: Unsupported (if the entity does not support the command)
- Code to try to prevent deadlocks

### Removed
- Fully removed acquire/lock state from descriptors, the controller only support it at entity level (globally)

### Fixed
- Preventing a crash in upper layers caused by toxic entities (Motu Ultralite card sending a GET_RX_STATE_RESPONSE with a non-existant stream index)
- Properly monitoring changes in dynamic mappings
- Updated GetMilanInfo to match Milan specification
- Dynamic mappings in redundancy mode not correctly set for StreamPortOutput

## [2.7.2] - 2018-10-30
### Fixed
- [Flagging not fully compliant entities as so, instead of discarding them](https://github.com/L-Acoustics/avdecc/issues/42)

## [2.7.1] - 2018-10-02
### Added
- [Retrieving entity current acquired state upon enumeration](https://github.com/L-Acoustics/avdecc/issues/26)
- [Support for GET_COUNTERS command and unsolicited notifications](https://github.com/L-Acoustics/avdecc/issues/12)

## [2.7.0] - 2018-09-12
### Added
- Support for MemoryObject Operations (Contributed by Florian Harmuth)

## Changed
- la::avdecc::controller::CompileOptions is now la::avdecc::EnumBitfield

## Fixed
- Prevented possible deadlocks if the caller has a lock on a ControlledEntity

## [2.6.3] - 2018-08-08
### Fixed
- [Incorrect StreamOutput channel dynamic mappings](https://github.com/L-Acoustics/avdecc/issues/30)
- [Queries scheduler might crash](https://github.com/L-Acoustics/avdecc/issues/19)

## [2.6.2] - 2018-08-06
### Fixed
- [macOS native crash during enumeration](https://github.com/L-Acoustics/avdecc/issues/27)

## [2.6.1] - 2018-07-30
### Added
- [Support for MemoryObject upload/download](https://github.com/L-Acoustics/avdecc/issues/18)

### Fixed
- [Dynamic mapping not properly enumerated](https://github.com/L-Acoustics/avdecc/issues/24)

## [2.6.0] - 2018-07-17
### Added
- Support for Memory Object descriptors (Contributed by Florian Harmuth)
- [Support for EntityModel cache](https://github.com/L-Acoustics/avdecc/issues/6)
- Notification callbacks for onAudioUnitNameChanged, onAvbInterfaceNameChanged, onClockSourceNameChanged, onMemoryObjectNameChanged, onAudioClusterNameChanged, onClockDomainNameChanged, onMemoryObjectLengthChanged

### Changed
- Every occurrence of vendorEntityModelID renamed entityModelID
- la::avdecc::controller::model::AvbInterfaceNodeStaticModel::clockIdentify renamed clockIdentity

## [2.5.3] - 2018-06-18

## [2.5.2] - 2018-06-15
### Fixed
- Issue when a device returns 0 as the total number of dynamic maps

## [2.5.1] - 2018-06-03
### Added
- Method to retrieve the controller's EID

## [2.5.0] - 2018-06-01
### Changed
- Advertising entities not supporting/implementing the query of dynamic information

### Fixed
- Removed noexcept qualifier from all std::function (c++17 compatibility)
- Ignoring incoherent 'connection count' field (Q6 Core after a reboot)

## [2.4.0] - 2018-05-15
### Added
- Global API to retrieve the compilation options of the library

### Fixed
- Some devices not properly enumerated if an OutputStream is connected
- Re-enabled AVnu Alliance Network Redundancy support

## [2.3.0] - 2018-03-30
### Added
- Support for Cole Peterson's redundant streams association
- Method to get a "lock guarded" ControlledEntity
- Support for Talker Stream Connections
- Method to forcefully remove a connection from a talker's StreamOutput (DisconnectTX spoofing)
- Support for GetAvbInfo command and updates
- Support for GetStreamInfo and GetAvbInfo unsolicited notifications
- Support for gptp information changed notification
- Helper API to get StreamRunning state

### Changed
- Full rework of entity model enumeration
- All completion and observer handlers are now triggered without keeping controller's mutex
- Full rework of ACMP notifications
- ACMP stream identification refactoring (new type describing the couple EntityID/StreamIndex)

### Fixed
- [Invalid descriptor type used to get dynamic audio mappings](https://github.com/L-Acoustics/avdecc/issues/4)

## [2.2.0] - 2017-12-01
### Added
- Support for setConfiguration command

## [2.1.0] - 2017-11-22
### Added
- setEntityName method, to change an entity's name (Entity Descriptor) and associated observer event
- setEntityGroupName method, to change an entity's group (Entity Descriptor) and associated observer event
- setConfigurationName method, to change a configuration's name (Configuration Descriptor) and associated observer event
- setStream[Input/Output]Name method, to change a stream's name (Stream Descriptor) and associated observer event
- Support for output stream dynamic audio mapping

### Changed
- la::avdecc::controller::Controller::Observer AECP notifications are now also triggered when the controller changes a value itself
- Removed onEntityAcquired and onEntityReleased events but added a more generic onAcquireStateChanged one

### Fixed
- [Entities declaring 0 stream input or stream output descriptors not properly detected](https://github.com/L-Acoustics/avdecc/issues/2)
- Not registering for unsolicited notifications on entities that do not support AEM

## [2.0.2] - 2017-10-27

## [2.0.0] - 2017-10-10
### Changed
- Rework of ControlledEntity class

## [1.0.0] - 2016-01-15
### Added
- Support for minimal working controller
