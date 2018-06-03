# Avdecc Library Changelog
All notable changes to the Avdecc Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.5.1] - 2018-06-03

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
