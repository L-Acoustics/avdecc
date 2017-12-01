# Avdecc Library Changelog
All notable changes to the Avdecc Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

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
