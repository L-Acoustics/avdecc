# Avdecc Controller Library Changelog
All notable changes to the Avdecc Controller Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
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
