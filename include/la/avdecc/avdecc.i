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
%std_function(AcquireEntityHandler, void, la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex);
%rename("$ignore") la::avdecc::entity::controller::Interface::releaseEntity; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::lockEntity; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::unlockEntity; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::queryEntityAvailable; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::queryControllerAvailable; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::registerUnsolicitedNotifications; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::unregisterUnsolicitedNotifications; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readEntityDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readConfigurationDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readAudioUnitDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readStreamInputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readStreamOutputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readJackInputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readJackOutputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readAvbInterfaceDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readClockSourceDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readMemoryObjectDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readLocaleDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readStringsDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readStreamPortInputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readStreamPortOutputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readExternalPortInputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readExternalPortOutputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readInternalPortInputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readInternalPortOutputDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readAudioClusterDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readAudioMapDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readControlDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::readClockDomainDescriptor; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setConfiguration; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getConfiguration; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setStreamInputFormat; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamInputFormat; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setStreamOutputFormat; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamOutputFormat; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamPortInputAudioMap; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamPortOutputAudioMap; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::addStreamPortInputAudioMappings; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::addStreamPortOutputAudioMappings; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::removeStreamPortInputAudioMappings; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::removeStreamPortOutputAudioMappings; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setStreamInputInfo; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setStreamOutputInfo; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamInputInfo; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamOutputInfo; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setEntityName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getEntityName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setEntityGroupName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getEntityGroupName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setConfigurationName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getConfigurationName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setAudioUnitName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAudioUnitName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setStreamInputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamInputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setStreamOutputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamOutputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setJackInputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getJackInputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setJackOutputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getJackOutputName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setAvbInterfaceName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAvbInterfaceName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setClockSourceName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getClockSourceName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setMemoryObjectName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getMemoryObjectName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setAudioClusterName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAudioClusterName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setControlName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getControlName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setClockDomainName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getClockDomainName; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setAssociation; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAssociation; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setAudioUnitSamplingRate; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAudioUnitSamplingRate; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setVideoClusterSamplingRate; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getVideoClusterSamplingRate; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setSensorClusterSamplingRate; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getSensorClusterSamplingRate; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setClockSource; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getClockSource; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setControlValues; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getControlValues; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::startStreamInput; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::startStreamOutput; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::stopStreamInput; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::stopStreamOutput; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAvbInfo; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAsPath; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getEntityCounters; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getAvbInterfaceCounters; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getClockDomainCounters; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamInputCounters; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getStreamOutputCounters; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::reboot; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::rebootToFirmware; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::startOperation; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::abortOperation; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::setMemoryObjectLength; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getMemoryObjectLength; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::addressAccess; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getMilanInfo; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::connectStream; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::disconnectStream; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::disconnectTalkerStream; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getTalkerStreamState; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getListenerStreamState; // Temp ignore method
%rename("$ignore") la::avdecc::entity::controller::Interface::getTalkerStreamConnection; // Temp ignore method

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
