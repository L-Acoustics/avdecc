/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file avdeccController.hpp
* @author Christophe Calmejane
* @brief Avdecc controller.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/internals/exception.hpp>
#include <la/avdecc/internals/entityModelTree.hpp>
#include <la/avdecc/internals/jsonSerialization.hpp>
#include <la/avdecc/memoryBuffer.hpp>

#include "internals/avdeccControlledEntity.hpp"
#include "internals/exports.hpp"

#include <memory>
#include <functional>
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <optional>

namespace la
{
namespace avdecc
{
namespace controller
{
/**
* Interface version of the library, used to check for compatibility between the version used to compile and the runtime version.<BR>
* Everytime the interface changes (what is visible from the user) you increase the InterfaceVersion value.<BR>
* A change in the visible interface is any modification in a public header file except a change in a private non-virtual method
* (either added, removed or signature modification).
* Any other change (including templates, inline methods, defines, typedefs, ...) are considered a modification of the interface.
*/
constexpr std::uint32_t InterfaceVersion = 400;

/**
* @brief Checks if the library is compatible with specified interface version.
* @param[in] interfaceVersion The interface version to check for compatibility.
* @return True if the library is compatible.
* @note If the library is not compatible, the application should no longer use the library.<BR>
*       When using the avdecc controller shared library, you must call this version to check the compatibility between the compiled and the loaded version.
*/
LA_AVDECC_CONTROLLER_API bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept;

/**
* @brief Gets the avdecc controller library version.
* @details Returns the avdecc controller library version.
* @return A string representing the library version.
*/
LA_AVDECC_CONTROLLER_API std::string LA_AVDECC_CONTROLLER_CALL_CONVENTION getVersion() noexcept;

/**
* @brief Gets the avdecc controller shared library interface version.
* @details Returns the avdecc controller shared library interface version.
* @return The interface version.
*/
LA_AVDECC_CONTROLLER_API std::uint32_t LA_AVDECC_CONTROLLER_CALL_CONVENTION getInterfaceVersion() noexcept;

enum class CompileOption : std::uint32_t
{
	None = 0,
	IgnoreNeitherStaticNorDynamicMappings = 1u << 0,
	EnableRedundancy = 1u << 15,
	Strict2018Redundancy = 1u << 16,
	EnableJsonSupport = 1u << 17,
};
using CompileOptions = utils::EnumBitfield<CompileOption>;

struct CompileOptionInfo
{
	CompileOption option{ CompileOption::None };
	std::string shortName{};
	std::string longName{};
};

/**
* @brief Gets the avdecc controller library compile options.
* @details Returns the avdecc controller library compile options.
* @return The compile options.
*/
LA_AVDECC_CONTROLLER_API CompileOptions LA_AVDECC_CONTROLLER_CALL_CONVENTION getCompileOptions() noexcept;

/**
* @brief Gets the avdecc controller library compile options info.
* @details Returns the avdecc controller library compile options info.
* @return The compile options info.
*/
LA_AVDECC_CONTROLLER_API std::vector<CompileOptionInfo> LA_AVDECC_CONTROLLER_CALL_CONVENTION getCompileOptionsInfo() noexcept;

/* ************************************************************************** */
/* Controller                                                                 */
/* ************************************************************************** */
/**
* @brief A Controller type entity.
* @details Controller handling local and remote entities discovery (ControlledEntity), state tracking every change in them and interacting with them through commands and queries.
*/
class Controller : public la::avdecc::utils::Subject<Controller, std::recursive_mutex>
{
public:
	using UniquePointer = std::unique_ptr<Controller, void (*)(Controller*)>;
	using DeviceMemoryBuffer = MemoryBuffer;
	static std::uint32_t constexpr ChecksumVersion = 2u;

	enum class Error
	{
		NoError = 0,
		InvalidProtocolInterfaceType = 1, /**< Selected protocol interface type is invalid */
		InterfaceOpenError = 2, /**< Failed to open interface. */
		InterfaceNotFound = 3, /**< Specified interface not found. */
		InterfaceInvalid = 4, /**< Specified interface is invalid. */
		DuplicateProgID = 5, /**< Specified ProgID is already in use on the local computer. */
		InvalidEntityModel = 6, /**< Provided EntityModel is invalid. */
		DuplicateExecutorName = 7, /**< Provided executor name already exists. */
		UnknownExecutorName = 8, /**< Provided executor name doesn't exist. */
		InternalError = 99, /**< Internal error, please report the issue. */
	};

	class LA_AVDECC_CONTROLLER_API Exception final : public la::avdecc::Exception
	{
	public:
		template<class T>
		Exception(Error const error, T&& text) noexcept
			: la::avdecc::Exception(std::forward<T>(text))
			, _error(error)
		{
		}
		Error getError() const noexcept
		{
			return _error;
		}

	private:
		Error const _error{ Error::NoError };
	};

	enum class QueryCommandError
	{
		RegisterUnsol,
		GetMilanInfo,
		EntityDescriptor,
		ConfigurationDescriptor,
		AudioUnitDescriptor,
		StreamInputDescriptor,
		StreamOutputDescriptor,
		JackInputDescriptor,
		JackOutputDescriptor,
		AvbInterfaceDescriptor,
		ClockSourceDescriptor,
		MemoryObjectDescriptor,
		LocaleDescriptor,
		StringsDescriptor,
		StreamPortInputDescriptor,
		StreamPortOutputDescriptor,
		AudioClusterDescriptor,
		AudioMapDescriptor,
		ControlDescriptor,
		ClockDomainDescriptor,
		TimingDescriptor,
		PtpInstanceDescriptor,
		PtpPortDescriptor,
		AcquiredState,
		LockedState,
		StreamInputAudioMap,
		StreamOutputAudioMap,
		TalkerStreamState,
		ListenerStreamState,
		TalkerStreamConnection,
		TalkerStreamInfo,
		ListenerStreamInfo,
		AvbInfo,
		AsPath,
		EntityCounters,
		AvbInterfaceCounters,
		ClockDomainCounters,
		StreamInputCounters,
		StreamOutputCounters,
		ConfigurationName,
		AudioUnitName,
		AudioUnitSamplingRate,
		InputStreamName,
		InputStreamFormat,
		OutputStreamName,
		OutputStreamFormat,
		InputJackName,
		OutputJackName,
		AvbInterfaceName,
		ClockSourceName,
		MemoryObjectName,
		MemoryObjectLength,
		AudioClusterName,
		ControlName,
		ControlValues,
		ClockDomainName,
		ClockDomainSourceIndex,
		TimingName,
		PtpInstanceName,
		PtpPortName,
	};

	/**
	* @brief Observer for entity state and query results. All handlers are guaranteed to be mutually exclusively called.
	* @warning For all handlers, the la::avdecc::controller::ControlledEntity parameter should not be copied, since there
	*          is no guaranty it will still be valid upon return (although it's guaranteed to be valid for the duration of
	*          the handler). If you later need to get a new temporary pointer to it call the getControlledEntity method.
	*/
	class Observer : public la::avdecc::utils::Observer<Controller>
	{
	public:
		virtual ~Observer() noexcept = default;

		// Global controller notifications
		virtual void onTransportError(la::avdecc::controller::Controller const* const controller) noexcept = 0;
		virtual void onEntityQueryError(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept = 0; // Might trigger even if entity is not "online" // Triggered when the controller failed to query all information it needs for an entity to be declared as Online

		// Discovery notifications (ADP)
		virtual void onEntityOnline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept = 0;
		virtual void onEntityOffline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept = 0;
		virtual void onEntityRedundantInterfaceOnline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::Entity::InterfaceInformation const& interfaceInfo) noexcept = 0;
		virtual void onEntityRedundantInterfaceOffline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex) noexcept = 0;
		virtual void onEntityCapabilitiesChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept = 0;
		virtual void onEntityAssociationIDChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept = 0;
		virtual void onGptpChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::UniqueIdentifier const grandMasterID, std::uint8_t const grandMasterDomain) noexcept = 0;

		// Global entity notifications
		virtual void onUnsolicitedRegistrationChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, bool const isSubscribed) noexcept = 0;
		virtual void onCompatibilityFlagsChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::ControlledEntity::CompatibilityFlags const compatibilityFlags) noexcept = 0;
		virtual void onIdentificationStarted(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept = 0;
		virtual void onIdentificationStopped(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept = 0;

		// Connection notifications (ACMP)
		virtual void onStreamInputConnectionChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInputConnectionInfo const& info, bool const changedByOther) noexcept = 0;
		virtual void onStreamOutputConnectionsChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamConnections const& connections) noexcept = 0;

		// Entity model notifications (unsolicited AECP or changes this controller sent)
		virtual void onAcquireStateChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::AcquireState const acquireState, la::avdecc::UniqueIdentifier const owningEntity) noexcept = 0;
		virtual void onLockStateChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::LockState const lockState, la::avdecc::UniqueIdentifier const lockingEntity) noexcept = 0;
		virtual void onStreamInputFormatChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept = 0;
		virtual void onStreamOutputFormatChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept = 0;
		virtual void onStreamInputDynamicInfoChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDynamicInfo const& info) noexcept = 0;
		virtual void onStreamOutputDynamicInfoChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDynamicInfo const& info) noexcept = 0;
		virtual void onEntityNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvdeccFixedString const& entityName) noexcept = 0;
		virtual void onEntityGroupNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept = 0;
		virtual void onConfigurationNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName) noexcept = 0;
		virtual void onAudioUnitNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName) noexcept = 0;
		virtual void onStreamInputNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamName) noexcept = 0;
		virtual void onStreamOutputNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamName) noexcept = 0;
		virtual void onJackInputNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackName) noexcept = 0;
		virtual void onJackOutputNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackName) noexcept = 0;
		virtual void onAvbInterfaceNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName) noexcept = 0;
		virtual void onClockSourceNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName) noexcept = 0;
		virtual void onMemoryObjectNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName) noexcept = 0;
		virtual void onAudioClusterNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName) noexcept = 0;
		virtual void onControlNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName) noexcept = 0;
		virtual void onClockDomainNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName) noexcept = 0;
		virtual void onTimingNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, la::avdecc::entity::model::AvdeccFixedString const& timingName) noexcept = 0;
		virtual void onPtpInstanceNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpInstanceName) noexcept = 0;
		virtual void onPtpPortNameChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpPortName) noexcept = 0;
		virtual void onAssociationIDChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::optional<la::avdecc::UniqueIdentifier> const associationID) noexcept = 0;
		virtual void onAudioUnitSamplingRateChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept = 0;
		virtual void onClockSourceChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex) noexcept = 0;
		virtual void onControlValuesChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::ControlValues const& controlValues) noexcept = 0;
		virtual void onStreamInputStarted(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
		virtual void onStreamOutputStarted(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
		virtual void onStreamInputStopped(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
		virtual void onStreamOutputStopped(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
		virtual void onAvbInterfaceInfoChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceInfo const& info) noexcept = 0;
		virtual void onAsPathChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AsPath const& asPath) noexcept = 0;
		virtual void onAvbInterfaceLinkStatusChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::controller::ControlledEntity::InterfaceLinkStatus const linkStatus) noexcept = 0;
		virtual void onEntityCountersChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::EntityCounters const& counters) noexcept = 0;
		virtual void onAvbInterfaceCountersChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceCounters const& counters) noexcept = 0;
		virtual void onClockDomainCountersChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockDomainCounters const& counters) noexcept = 0;
		virtual void onStreamInputCountersChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInputCounters const& counters) noexcept = 0;
		virtual void onStreamOutputCountersChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamOutputCounters const& counters) noexcept = 0;
		virtual void onMemoryObjectLengthChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept = 0;
		virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamPortIndex const streamPortIndex) noexcept = 0;
		virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamPortIndex const streamPortIndex) noexcept = 0;
		virtual void onOperationProgress(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, float const percentComplete) noexcept = 0; // A negative percentComplete value means the progress is unknown but still continuing
		virtual void onOperationCompleted(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, bool const failed) noexcept = 0;
		virtual void onMediaClockChainChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::controller::model::MediaClockChain const& mcChain) noexcept = 0;

		// Statistics
		/** When the count of AECP retry changed */
		virtual void onAecpRetryCounterChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept = 0;
		/** When the count of AECP timeout changed */
		virtual void onAecpTimeoutCounterChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept = 0;
		/** When the count of AECP unexpected response changed */
		virtual void onAecpUnexpectedResponseCounterChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept = 0;
		/** When the AECP average response time changed */
		virtual void onAecpResponseAverageTimeChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::chrono::milliseconds const& value) noexcept = 0;
		/** When the count of AEM-AECP unsolicited notifications changed */
		virtual void onAemAecpUnsolicitedCounterChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept = 0;
		/** When the count of lost AEM-AECP unsolicited notifications changed */
		virtual void onAemAecpUnsolicitedLossCounterChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept = 0;

		// Diagnostics
		virtual void onDiagnosticsChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::ControlledEntity::Diagnostics const& diags) noexcept = 0;
	};

	/** Defaulted version of Observer.*/
	class DefaultedObserver : public Observer
	{
	public:
		// Global controller notifications
		virtual void onTransportError(la::avdecc::controller::Controller const* const /*controller*/) noexcept override {}
		virtual void onEntityQueryError(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::Controller::QueryCommandError const /*error*/) noexcept override {} // Might trigger even if entity is not "online" // Triggered when the controller failed to query all information it needs for an entity to be declared as Online

		// Discovery notifications (ADP)
		virtual void onEntityOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept override {}
		virtual void onEntityOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept override {}
		virtual void onEntityRedundantInterfaceOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::Entity::InterfaceInformation const& /*interfaceInfo*/) noexcept override {}
		virtual void onEntityRedundantInterfaceOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/) noexcept override {}
		virtual void onEntityCapabilitiesChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept override {}
		virtual void onEntityAssociationIDChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept override {}
		virtual void onGptpChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::UniqueIdentifier const /*grandMasterID*/, std::uint8_t const /*grandMasterDomain*/) noexcept override {}

		// Global entity notifications
		virtual void onUnsolicitedRegistrationChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, bool const /*isSubscribed*/) noexcept override {}
		virtual void onCompatibilityFlagsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::ControlledEntity::CompatibilityFlags const /*compatibilityFlags*/) noexcept override {}
		virtual void onIdentificationStarted(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept override {}
		virtual void onIdentificationStopped(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/) noexcept override {}

		// Connection notifications (ACMP)
		virtual void onStreamInputConnectionChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInputConnectionInfo const& /*info*/, bool const /*changedByOther*/) noexcept override {}
		virtual void onStreamOutputConnectionsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamConnections const& /*connections*/) noexcept override {}

		// Entity model notifications (unsolicited AECP or changes this controller sent)
		virtual void onAcquireStateChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::AcquireState const /*acquireState*/, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept override {}
		virtual void onLockStateChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::LockState const /*lockState*/, la::avdecc::UniqueIdentifier const /*lockingEntity*/) noexcept override {}
		virtual void onStreamInputFormatChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept override {}
		virtual void onStreamOutputFormatChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept override {}
		virtual void onStreamInputDynamicInfoChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamDynamicInfo const& /*info*/) noexcept override {}
		virtual void onStreamOutputDynamicInfoChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamDynamicInfo const& /*info*/) noexcept override {}
		virtual void onEntityNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept override {}
		virtual void onEntityGroupNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept override {}
		virtual void onConfigurationNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept override {}
		virtual void onAudioUnitNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioUnitName*/) noexcept override {}
		virtual void onStreamInputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept override {}
		virtual void onStreamOutputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept override {}
		virtual void onJackInputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::JackIndex const /*jackIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*jackName*/) noexcept override {}
		virtual void onJackOutputNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::JackIndex const /*jackIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*jackName*/) noexcept override {}
		virtual void onAvbInterfaceNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*avbInterfaceName*/) noexcept override {}
		virtual void onClockSourceNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockSourceName*/) noexcept override {}
		virtual void onMemoryObjectNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*memoryObjectName*/) noexcept override {}
		virtual void onAudioClusterNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClusterIndex const /*audioClusterIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioClusterName*/) noexcept override {}
		virtual void onControlNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ControlIndex const /*controlIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*controlName*/) noexcept override {}
		virtual void onClockDomainNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockDomainName*/) noexcept override {}
		virtual void onTimingNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::TimingIndex const /*timingIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*timingName*/) noexcept override {}
		virtual void onPtpInstanceNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::PtpInstanceIndex const /*ptpInstanceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*ptpInstanceName*/) noexcept override {}
		virtual void onPtpPortNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::PtpPortIndex const /*ptpPortIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*ptpPortName*/) noexcept override {}
		virtual void onAssociationIDChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::optional<la::avdecc::UniqueIdentifier> const /*associationID*/) noexcept override {}
		virtual void onAudioUnitSamplingRateChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept override {}
		virtual void onClockSourceChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/) noexcept override {}
		virtual void onControlValuesChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ControlIndex const /*controlIndex*/, la::avdecc::entity::model::ControlValues const& /*controlValues*/) noexcept override {}
		virtual void onStreamInputStarted(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
		virtual void onStreamOutputStarted(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
		virtual void onStreamInputStopped(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
		virtual void onStreamOutputStopped(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
		virtual void onAvbInterfaceInfoChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInterfaceInfo const& /*info*/) noexcept override {}
		virtual void onAsPathChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AsPath const& /*asPath*/) noexcept override {}
		virtual void onAvbInterfaceLinkStatusChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::controller::ControlledEntity::InterfaceLinkStatus const /*linkStatus*/) noexcept override {}
		virtual void onEntityCountersChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::EntityCounters const& /*counters*/) noexcept override {}
		virtual void onAvbInterfaceCountersChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInterfaceCounters const& /*counters*/) noexcept override {}
		virtual void onClockDomainCountersChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockDomainCounters const& /*counters*/) noexcept override {}
		virtual void onStreamInputCountersChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInputCounters const& /*counters*/) noexcept override {}
		virtual void onStreamOutputCountersChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamOutputCounters const& /*counters*/) noexcept override {}
		virtual void onMemoryObjectLengthChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, std::uint64_t const /*length*/) noexcept override {}
		virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/) noexcept override {}
		virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/) noexcept override {}
		virtual void onOperationProgress(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, la::avdecc::entity::model::OperationID const /*operationID*/, float const /*percentComplete*/) noexcept override {} // A negative percentComplete value means the progress is unknown but still continuing
		virtual void onOperationCompleted(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, la::avdecc::entity::model::OperationID const /*operationID*/, bool const /*failed*/) noexcept override {}
		virtual void onMediaClockChainChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::controller::model::MediaClockChain const& /*mcChain*/) noexcept override {}

		// Statistics
		/** When the count of AECP retry changed */
		virtual void onAecpRetryCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override {}
		/** When the count of AECP timeout changed */
		virtual void onAecpTimeoutCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override {}
		/** When the count of AECP unexpected response changed */
		virtual void onAecpUnexpectedResponseCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override {}
		/** When the AECP average response time changed */
		virtual void onAecpResponseAverageTimeChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::chrono::milliseconds const& /*value*/) noexcept override {}
		/** When the count of AEM-AECP unsolicited notifications changed */
		virtual void onAemAecpUnsolicitedCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override {}
		/** When the count of lost AEM-AECP unsolicited notifications changed */
		virtual void onAemAecpUnsolicitedLossCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override {}

		// Diagnostics
		virtual void onDiagnosticsChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::ControlledEntity::Diagnostics const& /*diags*/) noexcept override {}
	};

	class ExclusiveAccessToken
	{
	public:
		using UniquePointer = std::unique_ptr<ExclusiveAccessToken, void (*)(ExclusiveAccessToken*)>;

		enum class AccessType
		{
			Acquire = 0,
			PersistentAcquire = 1,
			Lock = 2,
		};

		// Deleted compiler auto-generated methods
		ExclusiveAccessToken(ExclusiveAccessToken&&) = delete;
		ExclusiveAccessToken(ExclusiveAccessToken const&) = delete;
		ExclusiveAccessToken& operator=(ExclusiveAccessToken const&) = delete;
		ExclusiveAccessToken& operator=(ExclusiveAccessToken&&) = delete;

	protected:
		ExclusiveAccessToken() = default;
		virtual ~ExclusiveAccessToken() = default;
	};

	/* Enumeration and Control Protocol (AECP) AEM handlers. WARNING: The 'entity' parameter might be nullptr even if 'status' is AemCommandStatus::Success, in case the unit goes offline right after processing our command. */
	using AcquireEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity)>;
	using ReleaseEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity)>;
	using LockEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity)>;
	using UnlockEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity)>;
	using SetConfigurationHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamInputInfoHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamOutputInfoHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetEntityNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetAudioUnitNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamInputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetStreamOutputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetJackInputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetJackOutputNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetAvbInterfaceNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetClockSourceNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetMemoryObjectNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetAudioClusterNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetControlNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetClockDomainNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetTimingNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetPtpInstanceNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetPtpPortNameHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetAssociationIDHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetClockSourceHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetControlValuesHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StartStreamInputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StopStreamInputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using AddStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using AddStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using RemoveStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using RemoveStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using RebootHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using StartMemoryObjectOperationHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::OperationID const operationID)>;
	using AbortOperationHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using SetMemoryObjectLengthHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	using IdentifyEntityHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)>;
	/* Enumeration and Control Protocol (AECP) AA handlers. WARNING: The 'entity' parameter might be nullptr even if 'status' is AemCommandStatus::Success, in case the unit goes offline right after processing our command. */
	using ReadDeviceMemoryProgressHandler = std::function<bool(la::avdecc::controller::ControlledEntity const* const entity, float const percentComplete)>; // A negative percentComplete value means the progress is unknown but still continuing // Returning true will abort the operation
	using ReadDeviceMemoryCompletionHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AaCommandStatus const status, la::avdecc::controller::Controller::DeviceMemoryBuffer const& memoryBuffer)>;
	using WriteDeviceMemoryProgressHandler = std::function<bool(la::avdecc::controller::ControlledEntity const* const entity, float const percentComplete)>; // A negative percentComplete value means the progress is unknown but still continuing // Returning true will abort the operation
	using WriteDeviceMemoryCompletionHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AaCommandStatus const status)>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	using DisconnectTalkerStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status)>;
	/* Other handlers */
	using RequestExclusiveAccessResultHandler = std::function<void(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::controller::Controller::ExclusiveAccessToken::UniquePointer&& token)>;

	/**
	* @brief Factory method to create a new Controller.
	* @details Creates a new Controller as a unique pointer.
	* @param[in] protocolInterfaceType The protocol interface type to use.
	* @param[in] interfaceName The name of the interface to bind the controller to.
	*            Use #la::avdecc::networkInterface::enumerateInterfaces to get a list of valid
	*            interfaces, and pass the 'name' field of a #la::avdecc::networkInterface::Interface
	*            to this method.
	* @param[in] progID ID that will be used to generate the #UniqueIdentifier for this controller.
	* @param[in] entityModelID EntityModelID to publish for this controller. You can use entity::model::makeEntityModelID to create this value.
	* @param[in] preferedLocale ISO 639-1 locale code of the prefered locale to use when querying entity information.
	*                           If the specified locale is not found on the entity, then english is used.
	* @param[in] entityModelTree The entity model tree to use for this controller entity, or null to not expose a model.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages (must be created before the call). If empty, a default executor will be created.
	* @param[in] virtualEntityInterface The virtual entity interface to forward network calls to when manipulating a virtual entity, or null to use the network interface.
	* @return A new Controller as a Controller::UniquePointer.
	* @note Throws Exception if interfaceName is invalid or inaccessible, or if progID is already used on the local computer.
	*/
	static UniquePointer create(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree, std::optional<std::string> const& executorName, entity::controller::Interface const* const virtualEntityInterface)
	{
		auto deleter = [](Controller* controller)
		{
			controller->destroy();
		};
		return UniquePointer(createRawController(protocolInterfaceType, interfaceName, progID, entityModelID, preferedLocale, entityModelTree, executorName, virtualEntityInterface), deleter);
	}

	/** Returns the UniqueIdentifier this instance of the controller is using to identify itself on the network */
	virtual UniqueIdentifier getControllerEID() const noexcept = 0;

	/* Controller configuration methods */
	/** Enables entity advertising with available duration included between 2-62 seconds on the specified interfaceIndex if set, otherwise on all interfaces. Might throw an Exception. */
	virtual void enableEntityAdvertising(std::uint32_t const availableDuration, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) = 0;
	/** Disables entity advertising on the specified interfaceIndex if set, otherwise on all interfaces. */
	virtual void disableEntityAdvertising(std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) noexcept = 0;
	/** Requests a remote entities discovery. */
	virtual bool discoverRemoteEntities() const noexcept = 0;
	/** Requests a targetted remote entity discovery. */
	virtual bool discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept = 0;
	/** Sets automatic discovery delay. 0 (default) for no automatic discovery. */
	virtual void setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) noexcept = 0;
	/** Enables the EntityModel cache */
	virtual void enableEntityModelCache() noexcept = 0;
	/** Disables the EntityModel cache */
	virtual void disableEntityModelCache() noexcept = 0;
	/** Enables complete EntityModel (static part) enumeration. Depending on entities, it might take a much longer time to enumerate. */
	virtual void enableFullStaticEntityModelEnumeration() noexcept = 0;
	/** Disables complete EntityModel (static part) enumeration.*/
	virtual void disableFullStaticEntityModelEnumeration() noexcept = 0;

	/* Enumeration and Control Protocol (AECP) AEM. WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept = 0;
	virtual void unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept = 0;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept = 0;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& name, SetAudioUnitNameHandler const& handler) const noexcept = 0;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void setJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& name, SetJackInputNameHandler const& handler) const noexcept = 0;
	virtual void setJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& name, SetJackOutputNameHandler const& handler) const noexcept = 0;
	virtual void setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& name, SetAvbInterfaceNameHandler const& handler) const noexcept = 0;
	virtual void setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& name, SetClockSourceNameHandler const& handler) const noexcept = 0;
	virtual void setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& name, SetMemoryObjectNameHandler const& handler) const noexcept = 0;
	virtual void setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& name, SetAudioClusterNameHandler const& handler) const noexcept = 0;
	virtual void setControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& name, SetControlNameHandler const& handler) const noexcept = 0;
	virtual void setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& name, SetClockDomainNameHandler const& handler) const noexcept = 0;
	virtual void setTimingName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& name, SetTimingNameHandler const& handler) const noexcept = 0;
	virtual void setPtpInstanceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& name, SetPtpInstanceNameHandler const& handler) const noexcept = 0;
	virtual void setPtpPortName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& name, SetPtpPortNameHandler const& handler) const noexcept = 0;
	virtual void setAssociationID(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationIDHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept = 0;
	virtual void setControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept = 0;
	virtual void rebootToFirmware(UniqueIdentifier const targetEntityID, entity::model::MemoryObjectIndex const memoryObjectIndex, RebootHandler const& handler) const noexcept = 0;
	virtual void startStoreMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept = 0;
	virtual void startStoreAndRebootMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept = 0;
	virtual void startReadMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept = 0;
	virtual void startEraseMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept = 0;
	virtual void startUploadMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, std::uint64_t const dataLength, StartMemoryObjectOperationHandler const& handler) const noexcept = 0;
	virtual void abortOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept = 0;
	virtual void setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept = 0;
	virtual void identifyEntity(UniqueIdentifier const targetEntityID, std::chrono::milliseconds const duration, IdentifyEntityHandler const& handler) const noexcept = 0;

	/* Enumeration and Control Protocol (AECP) AA. WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void readDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, std::uint64_t const length, ReadDeviceMemoryProgressHandler const& progressHandler, ReadDeviceMemoryCompletionHandler const& completionHandler) const noexcept = 0;
	virtual void writeDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, la::avdecc::controller::Controller::DeviceMemoryBuffer memoryBuffer, WriteDeviceMemoryProgressHandler const& progressHandler, WriteDeviceMemoryCompletionHandler const& completionHandler) const noexcept = 0;

	/* Connection Management Protocol (ACMP). WARNING: The completion handler will not be called if the controller is destroyed while the query is inflight. Otherwise it will always be called. */
	virtual void connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept = 0;
	/** Sends a DisconnectTX message directly to the talker, spoofing the listener. Should only be used to forcefully disconnect a ghost connection on the talker. */
	virtual void disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept = 0;

	/** Gets a lock guarded ControlledEntity. While the returned object is in the scope, you are guaranteed to have exclusive access on the ControlledEntity. The returned guard should not be kept or held for more than a few milliseconds. */
	virtual ControlledEntityGuard getControlledEntityGuard(UniqueIdentifier const entityID) const noexcept = 0;

	/** Requests an ExclusiveAccessToken for the specified entityID. If the call succeeded (AemCommandStatus::Success), a valid token will be returned. The handler will always be called, either before the call returns or asynchronously. */
	virtual void requestExclusiveAccess(UniqueIdentifier const entityID, ExclusiveAccessToken::AccessType const type, RequestExclusiveAccessResultHandler&& handler) const noexcept = 0;

	/** BasicLockable concept 'lock' method for the whole Controller */
	virtual void lock() noexcept = 0;
	/** BasicLockable concept 'unlock' method for the whole Controller */
	virtual void unlock() noexcept = 0;

	/* Model serialization methods */
	/** Serializes all discovered ControlledEntities as JSON and save to specified file. If 'continueOnError' is specified and some error(s) occured, SerializationError::Incomplete will be returned. */
	virtual std::tuple<avdecc::jsonSerializer::SerializationError, std::string> serializeAllControlledEntitiesAsJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, std::string const& dumpSource, bool const continueOnError) const noexcept = 0;
	/** Serializes specified ControlledEntity as JSON and save to specified file. */
	virtual std::tuple<avdecc::jsonSerializer::SerializationError, std::string> serializeControlledEntityAsJson(UniqueIdentifier const entityID, std::string const& filePath, entity::model::jsonSerializer::Flags const flags, std::string const& dumpSource) const noexcept = 0;

	/* Model deserialization methods */
	/** Deserializes a JSON file representing a full network state, and loads it as virtual ControlledEntities. */
	virtual std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> loadVirtualEntitiesFromJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError) noexcept = 0;
	/** Deserializes a JSON file representing an entity, and loads it as a virtual ControlledEntity. */
	virtual std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> loadVirtualEntityFromJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags) noexcept = 0;
	/** Deserializes a JSON file representing a full network state, and returns the ControlledEntities without loading them. */
	static LA_AVDECC_CONTROLLER_API std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, std::vector<SharedControlledEntity>> LA_AVDECC_CONTROLLER_CALL_CONVENTION deserializeControlledEntitiesFromJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError) noexcept;
	/** Deserializes a JSON file representing an entity, and returns the ControlledEntity without loading it. */
	static LA_AVDECC_CONTROLLER_API std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, SharedControlledEntity> LA_AVDECC_CONTROLLER_CALL_CONVENTION deserializeControlledEntityFromJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags) noexcept;
	/** Loads an EntityModel file and feed it to the EntityModel cache */
	virtual std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> loadEntityModelFile(std::string const& filePath) noexcept = 0;

	/* Other helpful methods */
	/** Re-enumerates the specified entity (physical entity only). */
	virtual bool refreshEntity(UniqueIdentifier const entityID) noexcept = 0;
	/** Removes a Virtual Entity from the controller */
	virtual bool unloadVirtualEntity(UniqueIdentifier const entityID) noexcept = 0;
	/** Returns the StreamFormat among the provided availableFormats, that best matches desiredStreamFormat, using clockValidator delegate callback. Returns invalid StreamFormat if none is available. */
	static LA_AVDECC_CONTROLLER_API entity::model::StreamFormat LA_AVDECC_CONTROLLER_CALL_CONVENTION chooseBestStreamFormat(entity::model::StreamFormats const& availableFormats, entity::model::StreamFormat const desiredStreamFormat, std::function<bool(bool const isDesiredClockSync, bool const isAvailableClockSync)> const& clockValidator) noexcept;
	static LA_AVDECC_CONTROLLER_API bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isMediaClockStreamFormat(entity::model::StreamFormat const streamFormat) noexcept;
	/** Returns a checksum of the static entity model of the given ControlledEntity for the given checksumVersion (defaults to the maximum checksum version supported by the library). Nothing is returned if the model is invalid or incomplete (only full AEM enumeration yields a valid checksum) */
	static LA_AVDECC_CONTROLLER_API std::optional<std::string> LA_AVDECC_CONTROLLER_CALL_CONVENTION computeEntityModelChecksum(ControlledEntity const& controlledEntity, std::uint32_t const checksumVersion = ChecksumVersion) noexcept;

	// Deleted compiler auto-generated methods
	Controller(Controller const&) = delete;
	Controller(Controller&&) = delete;
	Controller& operator=(Controller const&) = delete;
	Controller& operator=(Controller&&) = delete;

	/** Destructor */
	virtual ~Controller() = default;

protected:
	/** Constructor */
	Controller() = default;

private:
	/** Create method for COM-like interface */
	static LA_AVDECC_CONTROLLER_API Controller* LA_AVDECC_CONTROLLER_CALL_CONVENTION createRawController(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree, std::optional<std::string> const& executorName, entity::controller::Interface const* const virtualEntityInterface);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

/* Operator overloads */
constexpr bool operator!(Controller::Error const error)
{
	return error == Controller::Error::NoError;
}

} // namespace controller
} // namespace avdecc
} // namespace la
