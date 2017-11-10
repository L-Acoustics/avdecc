# Avdecc Controller Library Changelog
All notable changes to the Avdecc Controller Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- setEntityName method, to change an entity's name (Entity Descriptor)
- setEntityGroupName method, to change an entity's group (Entity Descriptor)
- Support for output stream dynamic audio mapping
### Changed
- la::avdecc::controller::Controller::Observer AECP notifications are now also triggered when the controller changes a value itself
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
