# Avdecc Controller Library Changelog
All notable changes to the Avdecc Controller Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Support for JACK_INPUT/JACK_OUTPUT descriptors
- Support for CONTROL descriptors at AUDIO_UNIT, JACK, STREAM_PORT levels
- *numberOfValues* field in the ControlNode
- Detection of out-of-bounds MemoryObject length value
- isValid() method to ControlledEntityGuard
- Possibility to define a Proxy Interface to handle virtual entities
- Boolean value in ControlledEntity to know if unsolicited notifications are supported by the entity
- API to force refresh a entity (_reloadEntity(UniqueIdentifier)_)
- Support for UTF8 file paths
- New type of diagnostics: *controlCurrentValueOutOfBounds*

### Changed
- Complete controller entity model refactoring to support descriptors at non-configuration level
- EntityModelVisitor is now virtual pure, but a new derivated visitor (with all default implementation) has been added: DefaultedEntityModelVisitor
- la::avdecc::controller::Controller::Observer is now virtual pure, but a new derivated visitor (with all default implementation) has been added: la::avdecc::controller::Controller::DefaultedObserver
- [Executor name can be provided when creating a Controller](https://github.com/L-Acoustics/avdecc/issues/132)

### Removed
- Direct access to ClockSource descriptors from the ClockDomain. Will still be enumerated correctly when using the model visitor

### Fixed
- Controller entity model no longer uses pointers to prevent dangling issues when making copies
- Not flagging a device as non IEEE/Milan compatible, if the library cannot handle a CONTROL type if doesn't support
- CONTROL values updated by the device itself didn't trigger an update notification
- DynamicMappings were not being retrieved from the entities
- [Not flagging some devices as non-1722.1 compatible due to a control value out of bounds](https://github.com/L-Acoustics/avdecc/issues/134)
- [Detecting Identify Controls at JACK level](https://github.com/L-Acoustics/avdecc/issues/135)

## [3.4.1] - 2023-01-11
### Fixed
- [Crash when trying to get a ControlledEntity during OnPreAdvertise events](https://github.com/L-Acoustics/avdecc/issues/125)
- [Sometimes incorrectly detecting loss of unsol for Milan devices](https://github.com/L-Acoustics/avdecc/issues/126)

## [3.4.0] - 2023-01-05
### Added
- Checking validity of EntityModel given when creating a controller
- [Checking validity of EntityModel when discovering a device](https://github.com/L-Acoustics/avdecc/issues/80)
- [Detection for loss of unsolicited notifications for Milan devices](https://github.com/L-Acoustics/avdecc/issues/50)
- Diagnostics counter for when loss of unsolicited notifications is detected
- [API to compute the checksum of the AEM of a ControlledEntity](https://github.com/L-Acoustics/avdecc/issues/124)

## [3.3.0] - 2022-11-22
### Added
- [Computation of the Media Clock Chain for each controlled entity](https://github.com/L-Acoustics/avdecc/issues/111)
- Possibility to unload a virtual entity using `Controller::unloadVirtualEntity`
- A la::avdecc::entity::model::EntityTree can be fed into the controller so it responds to AEM query commands
- getAudioClusterNode method to ControlledEntity

### Changed
- Diagnostics::streamInputOverLatency is now a std::set and not a std::unordered_map with a bool as value

### Fixed
- [Virtual entities are loaded from the networking thread (executor)](https://github.com/L-Acoustics/avdecc/issues/110)
- Rare crash when an entity goes offline and online immediately again, when its ENTITY descriptor is being enumerated
- AvbInterface Link status properly initialized when loading a virtual entity
- No longer trying to load the AEM of a virtual entity that doesn't support it
- Crash when loading an incomplete ANS file

## [3.2.4] - 2022-07-08
### Added
- Controller::isMediaClockStreamFormat API
- virtualName to RedundantStreamNode (generated from underlying stream names)

### Fixed
- [Controller initiated Identify may deadlock](https://github.com/L-Acoustics/avdecc/issues/107)

## [3.2.3] - 2022-04-22
### Added
- Network diagnostics: msrp overdelay, redundancy warning
- New Compatibility value: MilanWithWarning

## [3.2.2] - 2022-01-20
### Added
- Method to retrieve the mappings that will become invalid (dangling) when a stream input format will change: _getStreamPortInputInvalidAudioMappingsForStreamFormat_
- Method to choose the best stream format among a list
- [Support for REBOOT command](https://github.com/L-Acoustics/avdecc/issues/99)
- [Entity diagnostics: msrp latency error](https://github.com/L-Acoustics/avdecc/issues/103)
- Entity diagnostics: Milan redundancy connection error
- New Compatibility flag (MilanWarning) for when a device is Milan Compatible but has some non critical warnings

### Fixed
- Broken managed virtual entities since 3.2.1

## [3.2.1] - 2021-12-10
### Added
- Possibility to deserialize virtual entities into ControlledEntity without adding them to the controller
  - `deserializeControlledEntitiesFromJsonNetworkState` for an ANS file
  - `deserializeControlledEntityFromJson` for an AVE file

## [3.2.0] - 2021-07-21
### Added
- Support for multiple Virtual Entities loading from FullNetworkState file
- More IEEE1722.1 compliance checks (same fields btw ADP and ENTITY_DESCRIPTOR)
- ControlledEntity::isIdentifying() method
- [setAssociationID and getAssociationID commands](https://github.com/L-Acoustics/avdecc/issues/32)

### Changed
- Renamed onEntityAssociationChanged to onEntityAssociationIDChanged

### Fixed
- [onIdentificationStarted correctly triggered if entity is in identification when discovered](https://github.com/L-Acoustics/avdecc/issues/93)
- [Not caching AEM for entities which have I/G bit set in their EntityModelID EUI-64](https://github.com/L-Acoustics/avdecc/issues/95)
- [AssociationID is now always stored as a std::optional<UniqueIdentifier>](https://github.com/L-Acoustics/avdecc/issues/79)

## [3.1.1] - 2021-04-02
### Fixed
- [Discard unsol notifications received before descriptor has been read](https://github.com/L-Acoustics/avdecc/issues/91)
- Detecting invalid Add/Remove mappings response from an entity

## [3.1.0] - 2021-01-18
### Added
- [onEntityRedundantInterfaceOnline/onEntityRedundantInterfaceOffline observer events](https://github.com/L-Acoustics/avdecc/issues/86)
- [Support for Control Descriptors (only Linear Values are currently supported)](https://github.com/L-Acoustics/avdecc/issues/88)
- [Support for Controller to Entity Identification](https://github.com/L-Acoustics/avdecc/issues/13)

### Fixed
- [Add/Remove StreaPort mappings validates protocol limits](https://github.com/L-Acoustics/avdecc/issues/84)
- [Better handling of cable redundancy](https://github.com/L-Acoustics/avdecc/issues/85)

## [3.0.2] - 2020-09-14
### Added
- Automatic discovery delay is now configurable

## [3.0.1] - 2020-05-25
### Fixed
- [Incorrect generated OUTPUT_STREAM connections list](https://github.com/L-Acoustics/avdecc/issues/77)

## [3.0.0] - 2020-03-10
### Changed
- Renamed onStreamConnectionChanged to onStreamInputConnectionChanged
- Renamed onStreamConnectionsChanged to onStreamOutputConnectionsChanged

## [2.11.3] - 2019-11-20

## [2.11.2] - 2019-11-08
### Added
- New informative (metadata) fields in json exports: A string representing the source of the dump

### Changed
- Dumped entities in Full Network State are always sorted by EntityID

### Fixed
- First block of WriteDeviceMemory sent twice

## [2.11.1] - 2019-10-02
### Added
- Support for Full Entity Enumeration (Static Model only)

### Changed
- More control over what is dumped when serializing an entity
- Renamed json dump APIs (there is now a flag to choose from readable and binary format)

## [2.11.0] - 2019-09-10
### Added
- Precompiled headers on windows
- ControlledEntity::getStreamPortInputNonRedundantAudioMappings (and Output equivalent)
- ControlledEntity StreamNodeDynamicModel now uses StreamDynamicInfo instead of StreamInfo
- ControlledEntity AvbInterfaceNodeDynamicModel now uses AvbInterfaceInfo instead of AvbInfo
- Controller::requestExclusiveAccess method

### Changed
- Always replacing complete StreamInfo data with the latest received (according to IEEE1722.1 clarification)

### Fixed
- [Incorrect usage of the AEM cache feature](https://github.com/L-Acoustics/avdecc/issues/62)
- ControlledEntityGuard move constructor not properly changing WatchDog information

## [2.10.0] - 2019-06-24
### Added
- Load of entity from readable json file and injection as virtual entity

### Removed
- Entity Model Tree definition (moved to low level library)

## [2.9.2] - 2019-05-20
### Added
- Support for Identify notifications
- Full entity and network state dump as readable json file
- Enumeration of STRINGS descriptor
- Support for Entity Descriptor counters

### Fixed
- Invalid strings indexes when not using the first locale
- Talker's connections list accurate again with Milan devices

## [2.9.1] - 2019-03-13
### Changed
- ControlledEntity visitor reports a more specialized parent class

### Fixed
- Controller entry-points automatically unlock all ControlledEntity (temporarily) during a sendMessage call
- StreamInput connected state not always properly updated for non-Milan devices, in FastConnect mode

## [2.9.0] - 2019-02-13
### Added
- Support for Milan STREAM_OUTPUT counters
- More Milan compatibility detection (Counters, AECP GET commands)

### Changed
- BAD_ARGUMENTS is allowed as a valid response for some AEM queries

### Fixed
- Uncaught exception when an enumeration error occured
- RegisterUnsolicitedNotification is allowed to return NO_RESOURCES (Milan spec)

## [2.8.0] - 2019-01-23
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
- Preventing a crash in upper layers caused by toxic entities (Motu Ultralite card sending a GET_RX_STATE_RESPONSE with a non-existent stream index)
- Properly monitoring changes in dynamic mappings
- Updated GetMilanInfo to match Milan specification
- Dynamic mappings in redundancy mode not correctly set for StreamPortOutput
- Concurrent access protection

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
