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
DEFINE_OPTIONAL_SIMPLE(OptDescriptorIndex, la::avdecc::entity::model::DescriptorIndex, avdeccEntityModel.getInvalidDescriptorIndex()) // Will be used for all kind of descriptor indexes
//DEFINE_OPTIONAL_SIMPLE(OptUInt8, std::uint8_t, (byte)0) // Not compiling in C# (Inconsistent accessibility)
//DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, ControlIndex, OptControlIndex, avdeccEntityModel.getInvalidDescriptorIndex())
//DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, AvbInterfaceIndex, OptAvbInterfaceIndex, avdeccEntityModel.getInvalidDescriptorIndex())
DEFINE_OPTIONAL_CLASS(la::avdecc, UniqueIdentifier, OptUniqueIdentifier, la.avdecc.UniqueIdentifier.getUninitializedUniqueIdentifier())
//DEFINE_OPTIONAL_CLASS(, std::uint8_t, OptUInt8, (byte)0) // Not compiling in C# (Inconsistent accessibility)

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

// TODO: DOESN'T SEEM TO BE CREATED!!!
%nspace la::avdecc::entity::controller::ControllerEntity;
%rename("%s") la::avdecc::entity::controller::ControllerEntity; // Unignore class
%ignore la::avdecc::entity::controller::ControllerEntity::ControllerEntity(ControllerEntity&&); // Ignore move constructor
%ignore la::avdecc::entity::controller::ControllerEntity::operator=; // Ignore copy operator

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
// DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, StreamDynamicInfo, OptStreamDynamicInfo, null)

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
DEFINE_AEM_TREE_NODE(Local);
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
%template(ConfigurationTrees) std::map<la::avdecc::entity::model::ConfigurationIndex, la::avdecc::entity::model::ConfigurationTree>;


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
