////////////////////////////////////////
// AVDECC LIBRARY SWIG file
////////////////////////////////////////

%module(directors="1") avdecc

%include <stl.i>
%include <std_string.i>
%include <std_set.i>
%include <stdint.i>
%include <std_pair.i>
%include <std_map.i>
%include <windows.i>
%include <std_unique_ptr.i>
#if 0
%include <swiginterface.i>
#endif
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif
%include "la/avdecc/internals/chrono.i"
%include "la/avdecc/internals/optional.i"
%include "la/avdecc/internals/std_function.i"

// Generated wrapper file needs to include our header file (include as soon as possible using 'insert(runtime)' as target language exceptions are defined early in the generated wrapper file)
%insert(runtime) %{
	#include <la/avdecc/memoryBuffer.hpp>
	#include <la/avdecc/executor.hpp>
	#include <la/avdecc/avdecc.hpp>
	#include <la/avdecc/internals/exception.hpp>
	#include <la/avdecc/internals/entity.hpp>
	#include <la/avdecc/internals/controllerEntity.hpp>
	#include <la/avdecc/internals/endStation.hpp>
%}

// Optimize code generation be enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
	$result = new $1_ltype((const $1_ltype &)$1);
%}

#define LA_AVDECC_API
#define LA_AVDECC_CALL_CONVENTION

////////////////////////////////////////
// MemoryBuffer class
////////////////////////////////////////
%nspace la::avdecc::MemoryBuffer;
%rename("isEqual") la::avdecc::MemoryBuffer::operator==;
%rename("isDifferent") la::avdecc::MemoryBuffer::operator!=;
%ignore la::avdecc::MemoryBuffer::operator bool; // Ignore operator bool, isValid() is already defined
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::MemoryBuffer::operator=;
// Ignore move constructor
%ignore la::avdecc::MemoryBuffer::MemoryBuffer(MemoryBuffer&&);
// Rename const data() getter
//%rename("constData") la::avdecc::MemoryBuffer::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBuffer::data(); // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBuffer::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER

#ifdef SWIGCSHARP
// Marshalling for void pointers
%apply unsigned char INPUT[]  { void const* const }
#endif

// Include c++ declaration file
%include "la/avdecc/memoryBuffer.hpp"


////////////////////////////////////////
// Executor/ExecutorManager classes
////////////////////////////////////////
%nspace la::avdecc::ExecutorManager;
%ignore la::avdecc::ExecutorManager::pushJob(std::string const& name, Executor::Job&& job); // Disable this version of the method, we'll wrap it
%ignore la::avdecc::ExecutorManager::isExecutorRegistered; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::registerExecutor; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::destroyExecutor; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::getExecutorThread; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::ExecutorWrapper; // RIGHT NOW IGNORE THIS CLASS
%ignore la::avdecc::Executor; // RIGHT NOW IGNORE THIS CLASS
%ignore la::avdecc::ExecutorWithDispatchQueue; // RIGHT NOW IGNORE THIS CLASS
%ignore la::avdecc::ExecutorProxy; // RIGHT NOW IGNORE THIS CLASS

%feature("director") la::avdecc::executorManager::PushJobHandler;
%nspace la::avdecc::executorManager::PushJobHandler;
%inline %{
	namespace la::avdecc::executorManager
	{
	class PushJobHandler
	{
		public:
			virtual void call() = 0;
			virtual ~PushJobHandler() {}
	};
	} // namespace la::avdecc::executorManager
%}

// Include c++ declaration file
%include "la/avdecc/executor.hpp"

// Define some wrapper functions
%extend la::avdecc::ExecutorManager
{
public:
    void pushJob(std::string const& name, executorManager::PushJobHandler* handler)
    {
        self->pushJob(name, [handler]()
        {
            if (handler)
            {
                handler->call();
            }
        });
    }
};


////////////////////////////////////////
// avdecc global
////////////////////////////////////////
%ignore la::avdecc::CompileOption;
%ignore la::avdecc::CompileOptionInfo;
%ignore la::avdecc::getCompileOptions();
%ignore la::avdecc::getCompileOptionsInfo();

// Include c++ declaration file
%include "la/avdecc/avdecc.hpp"


////////////////////////////////////////
// Exception class
////////////////////////////////////////
%nspace la::avdecc::Exception;
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::Exception::operator=;
// Ignore char* constructor
%ignore la::avdecc::Exception::Exception(char const* const);
// Ignore move constructor
%ignore la::avdecc::Exception::Exception(Exception&&);

// Include c++ declaration file
%include "la/avdecc/internals/exception.hpp"

////////////////////////////////////////
// Entity Model
////////////////////////////////////////
// Define optionals before including entityModel.i (we need to declare the optionals before the underlying types are defined)
DEFINE_OPTIONAL_SIMPLE(OptUInt8, std::uint8_t, (byte)0)
DEFINE_OPTIONAL_SIMPLE(OptUInt16, std::uint16_t, (ushort)0)
DEFINE_OPTIONAL_SIMPLE(OptUInt32, std::uint32_t, (uint)0)
DEFINE_OPTIONAL_SIMPLE(OptUInt64, std::uint64_t, (ulong)0)
//DEFINE_OPTIONAL_SIMPLE(OptDescriptorIndex, la::avdecc::entity::model::DescriptorIndex, avdeccEntityModel.getInvalidDescriptorIndex()) // Currently we cannot define both OptUInt16 and OptDescriptorIndex (or they mix up). We'll define each Descriptor type once we use a TypedDefine
DEFINE_OPTIONAL_SIMPLE(OptMsrpFailureCode, la::avdecc::entity::model::MsrpFailureCode, la.avdecc.entity.model.MsrpFailureCode.NoFailure)
DEFINE_OPTIONAL_CLASS(la::avdecc, UniqueIdentifier, OptUniqueIdentifier)
DEFINE_OPTIONAL_CLASS(la::networkInterface, MacAddress, OptMacAddress)

// Import entity model
%import "la/avdecc/internals/entityModel.i"


////////////////////////////////////////
// Entity/LocalEntity
////////////////////////////////////////
// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::entity::Entity;
%rename("%s") la::avdecc::entity::Entity; // Unignore class
%ignore la::avdecc::entity::Entity::Entity(Entity&&); // Ignore move constructor
%ignore la::avdecc::entity::Entity::operator=; // Ignore copy operator
%ignore la::avdecc::entity::Entity::getCommonInformation() const; // Ignore const overload
%ignore la::avdecc::entity::Entity::getInterfaceInformation(model::AvbInterfaceIndex const interfaceIndex) const; // Ignore const overload
%ignore la::avdecc::entity::Entity::getInterfacesInformation() const; // Ignore const overload
%nspace la::avdecc::entity::Entity::CommonInformation;
%rename("%s") la::avdecc::entity::Entity::CommonInformation; // Unignore child struct
%nspace la::avdecc::entity::Entity::InterfaceInformation;
%rename("%s") la::avdecc::entity::Entity::InterfaceInformation; // Unignore child struct

%nspace la::avdecc::entity::LocalEntity;
%rename("%s") la::avdecc::entity::LocalEntity; // Unignore class
%rename("lockEntity") la::avdecc::entity::LocalEntity::lock; // Rename method
%rename("unlockEntity") la::avdecc::entity::LocalEntity::unlock; // Rename method
%typemap(csbase) la::avdecc::entity::LocalEntity::AemCommandStatus "ushort"
%typemap(csbase) la::avdecc::entity::LocalEntity::AaCommandStatus "ushort"
%typemap(csbase) la::avdecc::entity::LocalEntity::MvuCommandStatus "ushort"
%typemap(csbase) la::avdecc::entity::LocalEntity::ControlStatus "ushort"
%typemap(csbase) la::avdecc::entity::LocalEntity::AdvertiseFlag "uint" // Currently hardcode as uint because of SWIG issue https://github.com/swig/swig/issues/2576
%rename("not") operator!(LocalEntity::AemCommandStatus const status); // Not put in a namespace https://github.com/swig/swig/issues/2459
%rename("or") operator|(LocalEntity::AemCommandStatus const lhs, LocalEntity::AemCommandStatus const rhs); // Not put in a namespace https://github.com/swig/swig/issues/2459
%ignore operator|=(LocalEntity::AemCommandStatus& lhs, LocalEntity::AemCommandStatus const rhs); // Don't know how to properly bind this with correct type defined (SWIG generates a SWIGTYPE_p file for this)
%rename("not") operator!(LocalEntity::AaCommandStatus const status); // Not put in a namespace https://github.com/swig/swig/issues/2459
%rename("or") operator|(LocalEntity::AaCommandStatus const lhs, LocalEntity::AaCommandStatus const rhs); // Not put in a namespace https://github.com/swig/swig/issues/2459
%ignore operator|=(LocalEntity::AaCommandStatus& lhs, LocalEntity::AaCommandStatus const rhs); // Don't know how to properly bind this with correct type defined (SWIG generates a SWIGTYPE_p file for this)
%rename("not") operator!(LocalEntity::MvuCommandStatus const status); // Not put in a namespace https://github.com/swig/swig/issues/2459
%rename("or") operator|(LocalEntity::MvuCommandStatus const lhs, LocalEntity::MvuCommandStatus const rhs); // Not put in a namespace https://github.com/swig/swig/issues/2459
%ignore operator|=(LocalEntity::MvuCommandStatus& lhs, LocalEntity::MvuCommandStatus const rhs); // Don't know how to properly bind this with correct type defined (SWIG generates a SWIGTYPE_p file for this)
%rename("not") operator!(LocalEntity::ControlStatus const status); // Not put in a namespace https://github.com/swig/swig/issues/2459
%rename("or") operator|(LocalEntity::ControlStatus const lhs, LocalEntity::ControlStatus const rhs); // Not put in a namespace https://github.com/swig/swig/issues/2459
%ignore operator|=(LocalEntity::ControlStatus& lhs, LocalEntity::ControlStatus const rhs); // Don't know how to properly bind this due to const overload
%ignore operator|=(LocalEntity::ControlStatus const lhs, LocalEntity::ControlStatus const rhs); // Don't know how to properly bind this due to const overload

// Include c++ declaration file
%include "la/avdecc/internals/entity.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
%template(InterfacesInformation) std::map<la::avdecc::entity::model::AvbInterfaceIndex, la::avdecc::entity::Entity::InterfaceInformation>;

////////////////////////////////////////
// ControllerEntity
////////////////////////////////////////
// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::entity::controller::Interface;
#if 1
%ignore la::avdecc::entity::controller::Interface; // TEMP IGNORE CLASS UNTIL WE CAN WRAP ALL RESULT HANDLERS
#else
%rename("%s") la::avdecc::entity::controller::Interface; // Unignore class
#endif
%ignore la::avdecc::entity::controller::Interface::Interface(Interface&&); // Ignore move constructor
%ignore la::avdecc::entity::controller::Interface::operator=; // Ignore copy operator
// TODO: Must wrap all result handlers

%nspace la::avdecc::entity::controller::Delegate;
%rename("%s") la::avdecc::entity::controller::Delegate; // Unignore class
%ignore la::avdecc::entity::controller::Delegate::Delegate(Delegate&&); // Ignore move constructor
%ignore la::avdecc::entity::controller::Delegate::operator=; // Ignore copy operator
%feature("director") la::avdecc::entity::controller::Delegate;
//%interface_impl(la::avdecc::entity::controller::Delegate); // Not working as expected

%nspace la::avdecc::entity::controller::Interface;
%rename("%s") la::avdecc::entity::controller::Interface; // Unignore class
%rename("%s") AcquireEntityHandler; // Unignore function // TODO: Would be nice to have the handler in the same namespace as the class (ie. be able to pass a namespace to std_function)
%rename("%s") ReleaseEntityHandler;
%rename("%s") LockEntityHandler;
%rename("%s") UnlockEntityHandler;
%rename("%s") QueryEntityAvailableHandler;
%rename("%s") QueryControllerAvailableHandler;
%rename("%s") RegisterUnsolicitedNotificationsHandler;
%rename("%s") UnregisterUnsolicitedNotificationsHandler;
%rename("%s") EntityDescriptorHandler;
%rename("%s") ConfigurationDescriptorHandler;
%rename("%s") AudioUnitDescriptorHandler;
%rename("%s") StreamInputDescriptorHandler;
%rename("%s") StreamOutputDescriptorHandler;
%rename("%s") JackInputDescriptorHandler;
%rename("%s") JackOutputDescriptorHandler;
%rename("%s") AvbInterfaceDescriptorHandler;
%rename("%s") ClockSourceDescriptorHandler;
%rename("%s") MemoryObjectDescriptorHandler;
%rename("%s") LocaleDescriptorHandler;
%rename("%s") StringsDescriptorHandler;
%rename("%s") StreamPortInputDescriptorHandler;
%rename("%s") StreamPortOutputDescriptorHandler;
%rename("%s") ExternalPortInputDescriptorHandler;
%rename("%s") ExternalPortOutputDescriptorHandler;
%rename("%s") InternalPortInputDescriptorHandler;
%rename("%s") InternalPortOutputDescriptorHandler;
%rename("%s") AudioClusterDescriptorHandler;
%rename("%s") AudioMapDescriptorHandler;
%rename("%s") ControlDescriptorHandler;
%rename("%s") ClockDomainDescriptorHandler;
%rename("%s") SetConfigurationHandler;
%rename("%s") GetConfigurationHandler;
%rename("%s") SetStreamInputFormatHandler;
%rename("%s") GetStreamInputFormatHandler;
%rename("%s") SetStreamOutputFormatHandler;
%rename("%s") GetStreamOutputFormatHandler;
%rename("%s") GetStreamPortInputAudioMapHandler;
%rename("%s") GetStreamPortOutputAudioMapHandler;
%rename("%s") AddStreamPortInputAudioMappingsHandler;
%rename("%s") AddStreamPortOutputAudioMappingsHandler;
%rename("%s") RemoveStreamPortInputAudioMappingsHandler;
%rename("%s") RemoveStreamPortOutputAudioMappingsHandler;
%rename("%s") SetStreamInputInfoHandler;
%rename("%s") SetStreamOutputInfoHandler;
%rename("%s") GetStreamInputInfoHandler;
%rename("%s") GetStreamOutputInfoHandler;
%rename("%s") SetEntityNameHandler;
%rename("%s") GetEntityNameHandler;
%rename("%s") SetEntityGroupNameHandler;
%rename("%s") GetEntityGroupNameHandler;
%rename("%s") SetConfigurationNameHandler;
%rename("%s") GetConfigurationNameHandler;
%rename("%s") SetAudioUnitNameHandler;
%rename("%s") GetAudioUnitNameHandler;
%rename("%s") SetStreamInputNameHandler;
%rename("%s") GetStreamInputNameHandler;
%rename("%s") SetStreamOutputNameHandler;
%rename("%s") GetStreamOutputNameHandler;
%rename("%s") SetJackInputNameHandler;
%rename("%s") GetJackInputNameHandler;
%rename("%s") SetJackOutputNameHandler;
%rename("%s") GetJackOutputNameHandler;
%rename("%s") SetAvbInterfaceNameHandler;
%rename("%s") GetAvbInterfaceNameHandler;
%rename("%s") SetClockSourceNameHandler;
%rename("%s") GetClockSourceNameHandler;
%rename("%s") SetMemoryObjectNameHandler;
%rename("%s") GetMemoryObjectNameHandler;
%rename("%s") SetAudioClusterNameHandler;
%rename("%s") GetAudioClusterNameHandler;
%rename("%s") SetControlNameHandler;
%rename("%s") GetControlNameHandler;
%rename("%s") SetClockDomainNameHandler;
%rename("%s") GetClockDomainNameHandler;
%rename("%s") SetAssociationHandler;
%rename("%s") GetAssociationHandler;
%rename("%s") SetAudioUnitSamplingRateHandler;
%rename("%s") GetAudioUnitSamplingRateHandler;
%rename("%s") SetVideoClusterSamplingRateHandler;
%rename("%s") GetVideoClusterSamplingRateHandler;
%rename("%s") SetSensorClusterSamplingRateHandler;
%rename("%s") GetSensorClusterSamplingRateHandler;
%rename("%s") SetClockSourceHandler;
%rename("%s") GetClockSourceHandler;
%rename("%s") SetControlValuesHandler;
%rename("%s") GetControlValuesHandler;
%rename("%s") StartStreamInputHandler;
%rename("%s") StartStreamOutputHandler;
%rename("%s") StopStreamInputHandler;
%rename("%s") StopStreamOutputHandler;
%rename("%s") GetAvbInfoHandler;
%rename("%s") GetAsPathHandler;
%rename("%s") GetEntityCountersHandler;
%rename("%s") GetAvbInterfaceCountersHandler;
%rename("%s") GetClockDomainCountersHandler;
%rename("%s") GetStreamInputCountersHandler;
%rename("%s") GetStreamOutputCountersHandler;
%rename("%s") RebootHandler;
%rename("%s") RebootToFirmwareHandler;
%rename("%s") StartOperationHandler;
%rename("%s") AbortOperationHandler;
%rename("%s") SetMemoryObjectLengthHandler;
%rename("%s") GetMemoryObjectLengthHandler;
%rename("%s") AddressAccessHandler;
%rename("%s") GetMilanInfoHandler;
%rename("%s") ConnectStreamHandler;
%rename("%s") DisconnectStreamHandler;
%rename("%s") DisconnectTalkerStreamHandler;
%rename("%s") GetTalkerStreamStateHandler;
%rename("%s") GetListenerStreamStateHandler;
%rename("%s") GetTalkerStreamConnectionHandler;
%std_function(AcquireEntityHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex);
%std_function(ReleaseEntityHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex);
%std_function(LockEntityHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex);
%std_function(UnlockEntityHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex);
%std_function(QueryEntityAvailableHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status);
%std_function(QueryControllerAvailableHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status);
%std_function(RegisterUnsolicitedNotificationsHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status);
%std_function(UnregisterUnsolicitedNotificationsHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status);
%std_function(EntityDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::EntityDescriptor const& descriptor);
%std_function(ConfigurationDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ConfigurationDescriptor const& descriptor);
%std_function(AudioUnitDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AudioUnitDescriptor const& descriptor);
%std_function(StreamInputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor);
%std_function(StreamOutputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor);
%std_function(JackInputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor);
%std_function(JackOutputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor);
%std_function(AvbInterfaceDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceDescriptor const& descriptor);
%std_function(ClockSourceDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::ClockSourceDescriptor const& descriptor);
%std_function(MemoryObjectDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::MemoryObjectDescriptor const& descriptor);
%std_function(LocaleDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::LocaleIndex const localeIndex, la::avdecc::entity::model::LocaleDescriptor const& descriptor);
%std_function(StringsDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StringsIndex const stringsIndex, la::avdecc::entity::model::StringsDescriptor const& descriptor);
%std_function(StreamPortInputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor);
%std_function(StreamPortOutputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor);
%std_function(ExternalPortInputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor);
%std_function(ExternalPortOutputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor);
%std_function(InternalPortInputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor);
%std_function(InternalPortOutputDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor);
%std_function(AudioClusterDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const clusterIndex, la::avdecc::entity::model::AudioClusterDescriptor const& descriptor);
%std_function(AudioMapDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMapDescriptor const& descriptor);
%std_function(ControlDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::ControlDescriptor const& descriptor);
%std_function(ClockDomainDescriptorHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockDomainDescriptor const& descriptor);
%std_function(SetConfigurationHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex);
%std_function(GetConfigurationHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex);
%std_function(SetStreamInputFormatHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat);
%std_function(GetStreamInputFormatHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat);
%std_function(SetStreamOutputFormatHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat);
%std_function(GetStreamOutputFormatHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat);
%std_function(GetStreamPortInputAudioMapHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings);
%std_function(GetStreamPortOutputAudioMapHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings);
%std_function(AddStreamPortInputAudioMappingsHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings);
%std_function(AddStreamPortOutputAudioMappingsHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings);
%std_function(RemoveStreamPortInputAudioMappingsHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings);
%std_function(RemoveStreamPortOutputAudioMappingsHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings);
%std_function(SetStreamInputInfoHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info);
%std_function(SetStreamOutputInfoHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info);
%std_function(GetStreamInputInfoHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info);
%std_function(GetStreamOutputInfoHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info);
%std_function(SetEntityNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName);
%std_function(GetEntityNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName);
%std_function(SetEntityGroupNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName);
%std_function(GetEntityGroupNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName);
%std_function(SetConfigurationNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName);
%std_function(GetConfigurationNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName);
%std_function(SetAudioUnitNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName);
%std_function(GetAudioUnitNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName);
%std_function(SetStreamInputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName);
%std_function(GetStreamInputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName);
%std_function(SetStreamOutputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName);
%std_function(GetStreamOutputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName);
%std_function(SetJackInputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackInputName);
%std_function(GetJackInputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackInputName);
%std_function(SetJackOutputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackOutputName);
%std_function(GetJackOutputNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackOutputName);
%std_function(SetAvbInterfaceNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName);
%std_function(GetAvbInterfaceNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName);
%std_function(SetClockSourceNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName);
%std_function(GetClockSourceNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName);
%std_function(SetMemoryObjectNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName);
%std_function(GetMemoryObjectNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName);
%std_function(SetAudioClusterNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName);
%std_function(GetAudioClusterNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName);
%std_function(SetControlNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName);
%std_function(GetControlNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName);
%std_function(SetClockDomainNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName);
%std_function(GetClockDomainNameHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName);
%std_function(SetAssociationHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const associationID);
%std_function(GetAssociationHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const associationID);
%std_function(SetAudioUnitSamplingRateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate);
%std_function(GetAudioUnitSamplingRateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate);
%std_function(SetVideoClusterSamplingRateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate);
%std_function(GetVideoClusterSamplingRateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate);
%std_function(SetSensorClusterSamplingRateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate);
%std_function(GetSensorClusterSamplingRateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate);
%std_function(SetClockSourceHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex);
%std_function(GetClockSourceHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex);
%std_function(SetControlValuesHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::MemoryBuffer const& packedControlValues);
%std_function(GetControlValuesHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::MemoryBuffer const& packedControlValues);
%std_function(StartStreamInputHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex);
%std_function(StartStreamOutputHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex);
%std_function(StopStreamInputHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex);
%std_function(StopStreamOutputHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex);
%std_function(GetAvbInfoHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info);
%std_function(GetAsPathHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AsPath const& asPath);
%std_function(GetEntityCountersHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::EntityCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters);
%std_function(GetAvbInterfaceCountersHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::AvbInterfaceCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters);
%std_function(GetClockDomainCountersHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::ClockDomainCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters);
%std_function(GetStreamInputCountersHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamInputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters);
%std_function(GetStreamOutputCountersHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamOutputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters);
%std_function(RebootHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status);
%std_function(RebootToFirmwareHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex);
%std_function(StartOperationHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, la::avdecc::entity::model::MemoryObjectOperationType const operationType, la::avdecc::MemoryBuffer const& memoryBuffer);
%std_function(AbortOperationHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID);
%std_function(SetMemoryObjectLengthHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length);
%std_function(GetMemoryObjectLengthHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length);
#if 1
%rename("$ignore") la::avdecc::entity::controller::Interface::addressAccess; // Temp ignore method
#else
%std_function(AddressAccessHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AaCommandStatus const status, la::avdecc::entity::addressAccess::Tlvs const& tlvs);
#endif
%std_function(GetMilanInfoHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::MvuCommandStatus const status, la::avdecc::entity::model::MilanInfo const& info);
%std_function(ConnectStreamHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(DisconnectStreamHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(DisconnectTalkerStreamHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(GetTalkerStreamStateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(GetListenerStreamStateHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(GetTalkerStreamConnectionHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);

%nspace la::avdecc::entity::ControllerEntity;
%rename("%s") la::avdecc::entity::ControllerEntity; // Unignore class
%ignore la::avdecc::entity::ControllerEntity::ControllerEntity(ControllerEntity&&); // Ignore move constructor
%ignore la::avdecc::entity::ControllerEntity::operator=; // Ignore copy operator

// Include c++ declaration file
%include "la/avdecc/internals/controllerEntity.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes


////////////////////////////////////////
// Protocol Interface
////////////////////////////////////////
#if 0 // Too much to define right now only to get ProtocolInterface::Type
// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::protocol::VuAecpdu;
%rename("%s") la::avdecc::protocol::VuAecpdu; // Unignore class

%nspace la::avdecc::protocol::ProtocolInterface;
%rename("%s") la::avdecc::protocol::ProtocolInterface; // Unignore class
//%ignore la::avdecc::protocol::ProtocolInterface::ProtocolInterface(ProtocolInterface&&); // Ignore move constructor
//%ignore la::avdecc::protocol::ProtocolInterface::operator=; // Ignore copy operator

// Include c++ declaration file
%include "la/avdecc/internals/protocolVuAecpdu.hpp"
%include "la/avdecc/internals/protocolInterface.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes
#endif


////////////////////////////////////////
// Entity Model Tree
////////////////////////////////////////
// Define some macros
%define DEFINE_AEM_TREE_COMMON(name)
	%nspace la::avdecc::entity::model::name;
	%rename("%s") la::avdecc::entity::model::name; // Unignore class
	%rename("isEqual") operator==(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	%rename("isDifferent") operator!=(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
%enddef
%define DEFINE_AEM_TREE_MODELS(name)
	%nspace la::avdecc::entity::model::name##NodeDynamicModel;
	%rename("%s") la::avdecc::entity::model::name##NodeDynamicModel; // Unignore class
	%nspace la::avdecc::entity::model::name##NodeStaticModel;
	%rename("%s") la::avdecc::entity::model::name##NodeStaticModel; // Unignore class
%enddef
%define DEFINE_AEM_TREE_NODE(name)
	%nspace la::avdecc::entity::model::name##Tree;
	%rename("%s") la::avdecc::entity::model::name##Tree; // Unignore class
%enddef
%define DEFINE_AEM_TREE_LEAF(name)
	%nspace la::avdecc::entity::model::name##NodeModels;
	%rename("%s") la::avdecc::entity::model::name##NodeModels; // Unignore class
%enddef

// Define optionals
DEFINE_OPTIONAL_SIMPLE(OptBool, bool, false)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, StreamDynamicInfo, OptStreamDynamicInfo)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, AvbInterfaceInfo, OptAvbInterfaceInfo)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, AsPath, OptAsPath)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, EntityCounters, OptEntityCounters)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, StreamInputCounters, OptStreamInputCounters)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, StreamOutputCounters, OptStreamOutputCounters)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, AvbInterfaceCounters, OptAvbInterfaceCounters)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, ClockDomainCounters, OptClockDomainCounters)

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable
DEFINE_AEM_TREE_COMMON(StreamInputConnectionInfo)
DEFINE_AEM_TREE_COMMON(StreamDynamicInfo)
DEFINE_AEM_TREE_COMMON(AvbInterfaceInfo)
DEFINE_AEM_TREE_MODELS(AudioUnit)
DEFINE_AEM_TREE_MODELS(Stream)
DEFINE_AEM_TREE_MODELS(StreamInput)
DEFINE_AEM_TREE_MODELS(StreamOutput)
DEFINE_AEM_TREE_MODELS(Jack)
DEFINE_AEM_TREE_MODELS(AvbInterface)
DEFINE_AEM_TREE_MODELS(ClockSource)
DEFINE_AEM_TREE_MODELS(MemoryObject)
DEFINE_AEM_TREE_MODELS(Locale)
DEFINE_AEM_TREE_MODELS(Strings)
DEFINE_AEM_TREE_MODELS(StreamPort)
DEFINE_AEM_TREE_MODELS(AudioCluster)
DEFINE_AEM_TREE_MODELS(AudioMap)
DEFINE_AEM_TREE_MODELS(Control)
DEFINE_AEM_TREE_MODELS(ClockDomain)
DEFINE_AEM_TREE_MODELS(Configuration)
DEFINE_AEM_TREE_MODELS(Entity)
DEFINE_AEM_TREE_LEAF(StreamInput);
DEFINE_AEM_TREE_LEAF(StreamOutput);
DEFINE_AEM_TREE_LEAF(AvbInterface);
DEFINE_AEM_TREE_LEAF(ClockSource);
DEFINE_AEM_TREE_LEAF(MemoryObject);
DEFINE_AEM_TREE_LEAF(Strings);
DEFINE_AEM_TREE_LEAF(AudioCluster);
DEFINE_AEM_TREE_LEAF(AudioMap);
DEFINE_AEM_TREE_LEAF(Control);
DEFINE_AEM_TREE_LEAF(ClockDomain);
DEFINE_AEM_TREE_NODE(Jack);
DEFINE_AEM_TREE_NODE(Locale);
DEFINE_AEM_TREE_NODE(StreamPort);
DEFINE_AEM_TREE_NODE(AudioUnit);
DEFINE_AEM_TREE_NODE(Configuration);
DEFINE_AEM_TREE_NODE(Entity);

// Include c++ declaration file
%include "la/avdecc/internals/entityModelTreeCommon.hpp"
%include "la/avdecc/internals/entityModelTreeDynamic.hpp"
%include "la/avdecc/internals/entityModelTreeStatic.hpp"
%include "la/avdecc/internals/entityModelTree.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
%template(StreamConnections) std::set<la::avdecc::entity::model::StreamIdentification>;
%template(ControlModels) std::map<la::avdecc::entity::model::ControlIndex, la::avdecc::entity::model::ControlNodeModels>;
%template(StringsModels) std::map<la::avdecc::entity::model::StringsIndex, la::avdecc::entity::model::StringsNodeModels>;
%template(AudioClusterModels) std::map<la::avdecc::entity::model::ClusterIndex, la::avdecc::entity::model::AudioClusterNodeModels>;
%template(AudioMapModels) std::map<la::avdecc::entity::model::MapIndex, la::avdecc::entity::model::AudioMapNodeModels>;
%template(StreamPortTrees) std::map<la::avdecc::entity::model::StreamPortIndex, la::avdecc::entity::model::StreamPortTree>;
%template(AudioUnitTrees) std::map<la::avdecc::entity::model::AudioUnitIndex, la::avdecc::entity::model::AudioUnitTree>;
%template(LocaleTrees) std::map<la::avdecc::entity::model::LocaleIndex, la::avdecc::entity::model::LocaleTree>;
%template(JackTrees) std::map<la::avdecc::entity::model::JackIndex, la::avdecc::entity::model::JackTree>;
%template(StreamInputModels) std::map<la::avdecc::entity::model::StreamIndex, la::avdecc::entity::model::StreamInputNodeModels>;
%template(StreamOutputModels) std::map<la::avdecc::entity::model::StreamIndex, la::avdecc::entity::model::StreamOutputNodeModels>;
%template(AvbInterfaceModels) std::map<la::avdecc::entity::model::AvbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceNodeModels>;
%template(ClockSourceModels) std::map<la::avdecc::entity::model::ClockSourceIndex, la::avdecc::entity::model::ClockSourceNodeModels>;
%template(MemoryObjectModels) std::map<la::avdecc::entity::model::MemoryObjectIndex, la::avdecc::entity::model::MemoryObjectNodeModels>;
%template(ClockDomainModels) std::map<la::avdecc::entity::model::ClockDomainIndex, la::avdecc::entity::model::ClockDomainNodeModels>;
%template(ConfigurationTrees) std::map<la::avdecc::entity::model::ConfigurationIndex, la::avdecc::entity::model::ConfigurationTree>;
%template(EntityCounters) std::map<la::avdecc::entity::EntityCounterValidFlag, la::avdecc::entity::model::DescriptorCounter>;
%template(StreamInputCounters) std::map<la::avdecc::entity::StreamInputCounterValidFlag, la::avdecc::entity::model::DescriptorCounter>;
%template(StreamOutputCounters) std::map<la::avdecc::entity::StreamOutputCounterValidFlag, la::avdecc::entity::model::DescriptorCounter>;
%template(AvbInterfaceCounters) std::map<la::avdecc::entity::AvbInterfaceCounterValidFlag, la::avdecc::entity::model::DescriptorCounter>;
%template(ClockDomainCounters) std::map<la::avdecc::entity::ClockDomainCounterValidFlag, la::avdecc::entity::model::DescriptorCounter>;
%template(LocalizedStrings) std::unordered_map<la::avdecc::entity::model::StringsIndex, la::avdecc::entity::model::AvdeccFixedString>;
//%template(DescriptorsVector) std::vector<la::avdecc::entity::model::DescriptorIndex>; // TODO: C# generated code doesn't compile

////////////////////////////////////////
// EndStation
////////////////////////////////////////
// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::EndStation;
%rename("%s") la::avdecc::EndStation; // Unignore class
%ignore la::avdecc::EndStation::Exception; // Ignore Exception, will be created as native exception
%ignore la::avdecc::EndStation::create(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceName); // Ignore it, will be wrapped
%unique_ptr(la::avdecc::EndStation) // Define unique_ptr for EndStation

// Define C# exception handling for la::avdecc::EndStation::Exception
%insert(runtime) %{
	typedef void (SWIGSTDCALL* EndStationExceptionCallback_t)(la::avdecc::EndStation::Error const error, char const* const message);
	EndStationExceptionCallback_t endStationExceptionCallback = NULL;

	extern "C" SWIGEXPORT void SWIGSTDCALL EndStationExceptionRegisterCallback(EndStationExceptionCallback_t exceptionCallback)
	{
		endStationExceptionCallback = exceptionCallback;
	}

	static void SWIG_CSharpSetPendingExceptionEndStation(la::avdecc::EndStation::Error const error, char const* const message)
	{
		endStationExceptionCallback(error, message);
	}
%}
%pragma(csharp) imclasscode=%{
	class EndStationExceptionHelper
	{
		public delegate void EndStationExceptionDelegate(la.avdecc.EndStationException.Error error, string message);
		static EndStationExceptionDelegate endStationDelegate = new EndStationExceptionDelegate(SetPendingEndStationException);

		[global::System.Runtime.InteropServices.DllImport(DllImportPath, EntryPoint="EndStationExceptionRegisterCallback")]
		public static extern void EndStationExceptionRegisterCallback(EndStationExceptionDelegate endStationDelegate);

		static void SetPendingEndStationException(la.avdecc.EndStationException.Error error, string message)
		{
			SWIGPendingException.Set(new la.avdecc.EndStationException(error, message));
		}

		static EndStationExceptionHelper()
		{
			EndStationExceptionRegisterCallback(endStationDelegate);
		}
	}
	static EndStationExceptionHelper exceptionHelper = new EndStationExceptionHelper();
%}
%pragma(csharp) moduleimports=%{
namespace la.avdecc
{
	class EndStationException : global::System.ApplicationException
	{
		public enum Error
		{
			NoError = 0,
			InvalidProtocolInterfaceType = 1, /**< Selected protocol interface type is invalid. */
			InterfaceOpenError = 2, /**< Failed to open interface. */
			InterfaceNotFound = 3, /**< Specified interface not found. */
			InterfaceInvalid = 4, /**< Specified interface is invalid. */
			DuplicateEntityID = 5, /**< EntityID not available (either duplicate, or no EntityID left on the local computer). */
			InvalidEntityModel = 6, /**< Provided EntityModel is invalid. */
			InternalError = 99, /**< Internal error, please report the issue. */
		}
		public EndStationException(Error error, string message)
			: base(message)
		{
			_error = error;
		}
		Error getError()
		{
			return _error;
		}
		private Error _error = Error.NoError;
	}
}
%}
// Throw typemap
%typemap (throws, canthrow=1) la::avdecc::EndStation::Exception %{
	SWIG_CSharpSetPendingExceptionEndStation($1.getError(), $1.what());
	return $null;
%}

// Define catches for methods that can throw
%catches(la::avdecc::EndStation::Exception) la::avdecc::EndStation::createEndStation;

// Include c++ declaration file
%include "la/avdecc/internals/endStation.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define wrapped functions
%extend la::avdecc::EndStation
{
public:
	static std::unique_ptr<la::avdecc::EndStation> createEndStation(/*protocol::ProtocolInterface::Type const protocolInterfaceType, */std::string const& networkInterfaceName)
	{
		try
		{
			// Right now, force PCap as we cannot bind the protocolInterfaceType enum correctly
			return std::unique_ptr<la::avdecc::EndStation>{ la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::PCap, networkInterfaceName).release() };
		}
		catch (la::avdecc::EndStation::Exception const& e)
		{
			SWIG_CSharpSetPendingExceptionEndStation(e.getError(), e.what());
			return nullptr;
		}
	}
};
